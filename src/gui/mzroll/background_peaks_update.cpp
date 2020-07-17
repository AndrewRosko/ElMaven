#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>

#include <QDir>
#include <QCoreApplication>
#include <QProcess>
#include <QJsonObject>

#include "Compound.h"
#include "adductdetection.h"
#include "alignmentdialog.h"
#include "common/analytics.h"
#include "csvreports.h"
#include "background_peaks_update.h"
#include "database.h"
#include "groupFiltering.h"
#include "grouprtwidget.h"
#include "isotopeDetection.h"
#include "mainwindow.h"
#include "masscutofftype.h"
#include "mavenparameters.h"
#include "mzAligner.h"
#include "mzSample.h"
#include "obiwarp.h"
#include "PeakDetector.h"
#include "samplertwidget.h"
#include "EIC.h"

BackgroundPeakUpdate::BackgroundPeakUpdate(QWidget*) {
        mainwindow = NULL;
        _stopped = true;
        setTerminationEnabled(true);
        runFunction = "computeKnowsPeaks";
        peakDetector = nullptr;
        setPeakDetector(new PeakDetector());
}

BackgroundPeakUpdate::~BackgroundPeakUpdate() {
    delete peakDetector;
    mavenParameters->cleanup();  // remove allgroups
}

/**
 * BackgroundPeakUpdate::run This function starts the thread. This function is
 * called by start() internally in QTThread. start() function will be called
 * where the thread starts.
 */
void BackgroundPeakUpdate::run(void) {
        //Making sure that instance of the mainwindow is present so that the
        //peakdetection process can be ran
        if (mainwindow == NULL) {
                quit();
                return;
        }
        connect(this, SIGNAL(alignmentError(QString)), mainwindow, SLOT(showAlignmentErrorDialog(QString)));
        if (mavenParameters->alignSamplesFlag) {
                connect(this, SIGNAL(alignmentComplete(QList<PeakGroup> )), mainwindow, SLOT(showAlignmentWidget()));
        }
	qRegisterMetaType<QList<PeakGroup> >("QList<PeakGroup>");
	connect(this, SIGNAL(alignmentComplete(QList<PeakGroup> )), mainwindow, SLOT(plotAlignmentVizAllGroupGraph(QList<PeakGroup>)));
	connect(this, SIGNAL(alignmentComplete(QList<PeakGroup> )), mainwindow->groupRtWidget, SLOT(setCurrentGroups(QList<PeakGroup>)));
        connect(this, SIGNAL(alignmentComplete(QList<PeakGroup> )), mainwindow->sampleRtWidget, SLOT(plotGraph()));
        mavenParameters->stop = false;
        started();
        if (runFunction == "alignUsingDatabase") {
                alignUsingDatabase();
        } else if (runFunction == "processSlices") {
            processSlices();
        } else if (runFunction == "processMassSlices") {
                processMassSlices();
        } else if (runFunction == "pullIsotopes") {
                pullIsotopes(mavenParameters->_group);
        } else if (runFunction == "pullIsotopesBarPlot") {
                pullIsotopesBarPlot(mavenParameters->_group);
        } else if (runFunction == "computePeaks") {
                computePeaks();
        } else if(runFunction == "alignWithObiWarp" ){
                alignWithObiWarp();
        } else {
                qDebug() << "Unknown Function " << runFunction.c_str();
        }

        quit();
        return;
}
void BackgroundPeakUpdate::alignWithObiWarp()
{
    ObiParams *obiParams = new ObiParams(mainwindow->alignmentDialog->scoreObi->currentText().toStdString(),
                                         mainwindow->alignmentDialog->local->isChecked(),
                                         mainwindow->alignmentDialog->factorDiag->value(),
                                         mainwindow->alignmentDialog->factorGap->value(),
                                         mainwindow->alignmentDialog->gapInit->value(),
                                         mainwindow->alignmentDialog->gapExtend->value(),
                                         mainwindow->alignmentDialog->initPenalty->value(),
                                         mainwindow->alignmentDialog->responseObiWarp->value(),
                                         mainwindow->alignmentDialog->noStdNormal->isChecked(),
                                         mainwindow->alignmentDialog->binSizeObiWarp->value());

    Q_EMIT(updateProgressBar("Aligning samples…", 0, 100));

    Aligner aligner;
    aligner.setAlignmentProgress.connect(boost::bind(&BackgroundPeakUpdate::qtSlot,
                                                     this, _1, _2, _3));

    _stopped = aligner.alignWithObiWarp(mavenParameters->samples, obiParams, mavenParameters);
    delete obiParams;

    if (_stopped) {
        Q_EMIT(restoreAlignment());
        //restore previous RTs
        for (auto sample : mavenParameters->samples) {
            sample->restorePreviousRetentionTimes();
        }

        //stopped without user intervention
        if (!mavenParameters->stop)
            Q_EMIT(alignmentError(
                   QString("There was an error during alignment. Please try again.")));

        mavenParameters->stop = false;
        return;
    }
        
    mainwindow->sampleRtWidget->plotGraph();
    Q_EMIT(samplesAligned(true));

}
void BackgroundPeakUpdate::emitGroups()
{
    peakDetector->pullAllIsotopes();
    for (PeakGroup& group : mavenParameters->allgroups) {
        if (mavenParameters->keepFoundGroups) {
            emit newPeakGroup(&group);
            QCoreApplication::processEvents();
        }
    }
}

void BackgroundPeakUpdate::setPeakDetector(PeakDetector *pd)
{
    if (peakDetector != nullptr)
        delete peakDetector;

    peakDetector = pd;
    peakDetector->boostSignal.connect(boost::bind(&BackgroundPeakUpdate::qtSlot,
                                                  this,
                                                  _1,
                                                  _2,
                                                  _3));
}

void BackgroundPeakUpdate::processSlices() {
        processSlices(mavenParameters->_slices, "sliceset");
}

void BackgroundPeakUpdate::processSlice(mzSlice& slice) {
        vector<mzSlice*> slices;
        slices.push_back(&slice);
        processSlices(slices, "sliceset");
}

//TODO: kiran Make a function which tell that its from option
//window and should be called where the settings are been called
void BackgroundPeakUpdate::getProcessSlicesSettings() {
        QSettings* settings = mainwindow->getSettings();

        // To Do: Are these lines required. The same is already being done in PeakDetectionDialog.cpp
        // mavenParameters->baseline_smoothingWindow = settings->value(
        //         "baseline_smoothing").toInt();
        // mavenParameters->baseline_dropTopX =
        //         settings->value("baseline_quantile").toInt();

}

void BackgroundPeakUpdate::align() {

        //These else if statements will take care of all corner cases of undoAlignment
        if (mavenParameters->alignSamplesFlag && mavenParameters->alignButton > 0) {
                ;
        } else if (mavenParameters->alignSamplesFlag && mavenParameters->alignButton ==0){
                mavenParameters->alignButton++;
                mavenParameters->undoAlignmentGroups = mavenParameters->allgroups;
        } else if (mavenParameters->alignSamplesFlag && mavenParameters->alignButton == -1) {
                ;
        } else {
                mavenParameters->alignButton = -1;
                mavenParameters->undoAlignmentGroups = mavenParameters->allgroups;
        }

        if (mavenParameters->alignSamplesFlag && !mavenParameters->stop) {
                Q_EMIT(updateProgressBar("Aligning samples…", 0, 0));
                vector<PeakGroup*> groups(mavenParameters->allgroups.size());
                for (int i = 0; i < mavenParameters->allgroups.size(); i++)
                        groups[i] = &mavenParameters->allgroups[i];
                Aligner aligner;
                int alignAlgo = mainwindow->alignmentDialog->alignAlgo->currentIndex();

                if (alignAlgo == 1) {
                        mainwindow->getAnalytics()->hitEvent("Alignment", "PolyFit");
                        aligner.setMaxIterations(mainwindow->alignmentDialog->maxIterations->value());
                        aligner.setPolymialDegree(mainwindow->alignmentDialog->polynomialDegree->value());
                        aligner.doAlignment(groups);
                        mainwindow->sampleRtWidget->setDegreeMap(aligner.sampleDegree);
                        mainwindow->sampleRtWidget->setCoefficientMap(aligner.sampleCoefficient);
                }

                mainwindow->deltaRt = aligner.getDeltaRt();
                mavenParameters->alignSamplesFlag = false;

        }
        QList<PeakGroup> listGroups;
        for (unsigned int i = 0; i<mavenParameters->allgroups.size(); i++) {
                listGroups.append(mavenParameters->allgroups.at(i));
        }	

        Q_EMIT(alignmentComplete(listGroups));
        Q_EMIT(samplesAligned(true));
}

void BackgroundPeakUpdate::alignUsingDatabase() {

    vector<mzSlice*> slices =
        peakDetector->processCompounds(mavenParameters->compounds, "compounds");
        processSlices(slices, "compounds");


}

void BackgroundPeakUpdate::processSlices(vector<mzSlice*>&slices,
                                         string setName) {
        getProcessSlicesSettings();

        peakDetector->processSlices(slices, setName);

        if (runFunction == "alignUsingDatabase") align();

        // filter for the best spectral matches per unique group
        if (mavenParameters->matchFragmentationFlag) {
            updateProgressBar("Filtering for best MS/MS annotations…", 0, 0);
            GroupFiltering groupFiltering(mavenParameters);
            groupFiltering.filterAllButSome(mavenParameters->allgroups,
                                            GroupFiltering::FilterType::MsMsScore,
                                            mavenParameters->fragAnnotationLimit);
        }

        if (mavenParameters->showProgressFlag
            && mavenParameters->pullIsotopesFlag) {
                Q_EMIT(updateProgressBar("Calculating Isotopes", 1, 100));
        }

        if (mavenParameters->searchAdducts) {
            mavenParameters->allgroups = AdductDetection::findAdducts(
                mavenParameters->allgroups,
                mavenParameters->getChosenAdductList(),
                peakDetector);

            // we remove adducts for which parent ion's RT is too different
            AdductDetection::filterAdducts(mavenParameters->allgroups,
                                           mavenParameters);
            emit (updateProgressBar("Filtering out false adducts…", 0, 0));
        }
        emitGroups();
}

void BackgroundPeakUpdate::qtSlot(const string& progressText, unsigned int progress, int totalSteps)
{
        Q_EMIT(updateProgressBar(QString::fromStdString(progressText), progress, totalSteps));

}

void BackgroundPeakUpdate::processCompounds(vector<Compound*> set,
                                            string setName)
{
    if (set.size() == 0)
            return;

    Q_EMIT(updateProgressBar("Processing Compounds", 0, 0));

    vector<mzSlice*> slices = peakDetector->processCompounds(set, setName);
    processSlices(slices, setName);
    delete_all(slices);
    Q_EMIT(updateProgressBar("Status", 0, 100));
}

void BackgroundPeakUpdate::processMassSlices() {
        Q_EMIT (updateProgressBar("Computing Mass Slices", 0, 0));
        mavenParameters->sig.connect(boost::bind(&BackgroundPeakUpdate::qtSignalSlot, this, _1, _2, _3));
        peakDetector->processMassSlices(mavenParameters->compounds);

        // filter for the best spectral matches per unique group
        if (mavenParameters->matchFragmentationFlag) {
            updateProgressBar("Filtering for best MS/MS annotations…", 0, 0);
            GroupFiltering groupFiltering(mavenParameters);
            groupFiltering.filterAllButSome(mavenParameters->allgroups,
                                            GroupFiltering::FilterType::MsMsScore,
                                            mavenParameters->fragAnnotationLimit);
        }

        align();

        emitGroups();
        Q_EMIT(updateProgressBar("Status", 0, 100));
}

void BackgroundPeakUpdate::qtSignalSlot(const string& progressText, unsigned int completed_slices, int total_slices)
{
        Q_EMIT(updateProgressBar(QString::fromStdString(progressText), completed_slices, total_slices));

}

void BackgroundPeakUpdate::completeStop()
{
    peakDetector->resetProgressBar();
    mavenParameters->stop = true;
    stop();
}

void BackgroundPeakUpdate::computePeaks() {

        processCompounds(mavenParameters->compounds, "groups");
}

/**
 * BackgroundPeakUpdate::setRunFunction Getting the function that has tobe ran
 * as a thread and updating it inside a variable
 * @param functionName [description]
 */
void BackgroundPeakUpdate::setRunFunction(QString functionName) {
        runFunction = functionName.toStdString();
}

void BackgroundPeakUpdate::pullIsotopes(PeakGroup* parentgroup)
{
    parentgroup->computeFragPattern(mavenParameters->fragmentTolerance);
    parentgroup->matchFragmentation(mavenParameters->fragmentTolerance,
                                    mavenParameters->scoringAlgo);

	bool isotopeFlag = mavenParameters->pullIsotopesFlag;
    if (!isotopeFlag)
        return;

	bool C13Flag = mavenParameters->C13Labeled_BPE;
	bool N15Flag = mavenParameters->N15Labeled_BPE;
	bool S34Flag = mavenParameters->S34Labeled_BPE;
	bool D2Flag = mavenParameters->D2Labeled_BPE;

        IsotopeDetection::IsotopeDetectionType isoType;
        isoType = IsotopeDetection::PeakDetection;

	IsotopeDetection isotopeDetection(
                mavenParameters,
                isoType,
                C13Flag,
                N15Flag,
                S34Flag,
                D2Flag);
	isotopeDetection.pullIsotopes(parentgroup);
}

void BackgroundPeakUpdate::pullIsotopesBarPlot(PeakGroup* parentgroup) {

	bool C13Flag = mavenParameters->C13Labeled_Barplot;
	bool N15Flag = mavenParameters->N15Labeled_Barplot;
	bool S34Flag = mavenParameters->S34Labeled_Barplot;
	bool D2Flag = mavenParameters->D2Labeled_Barplot;

        IsotopeDetection::IsotopeDetectionType isoType;
        isoType = IsotopeDetection::BarPlot;

	IsotopeDetection isotopeDetection(
                mavenParameters,
                isoType,
                C13Flag,
                N15Flag,
                S34Flag,
                D2Flag);
        isotopeDetection.pullIsotopes(parentgroup);
}

bool BackgroundPeakUpdate::covertToMzXML(QString filename, QString outfile) {

        QFile test(outfile);
        if (test.exists())
                return true;

        QString command = QString("ReAdW.exe --centroid --mzXML \"%1\" \"%2\"").arg(
                filename).arg(outfile);

        qDebug() << command;

        QProcess *process = new QProcess();
        //connect(process, SIGNAL(finished(int)), this, SLOT(doVideoCreated(int)));
        process->start(command);

        if (!process->waitForStarted()) {
                process->kill();
                return false;
        }

        while (!process->waitForFinished()) {
        };
        QFile testOut(outfile);
        return testOut.exists();
}

void BackgroundPeakUpdate::updateGroups(QList<shared_ptr<PeakGroup>> groups,
                                        vector<mzSample *> samples,
                                        MavenParameters* mavenParameters)
{

    
    for(auto group : groups)
    {
        MavenParameters* mp = group->parameters().get();
        auto slice = group->getSlice();
        slice.rtmin = samples[0]->minRt;
        slice.rtmax = samples[0]->maxRt;

        auto eics  = PeakDetector::pullEICs(&slice,
                                           samples,
                                           mp);
        for(auto eic : eics)
        {
            for(Peak& peak :  group->peaks)
            {
                if (eic->getSample() ==
                    peak.getSample()){
                    eic->getPeakDetails(peak);
                }
            }
        }
        group->groupStatistics();

        if (!group->isIsotope() && group->childCount() > 0)
        {
            group->children.clear();
            bool C13Flag = mp->C13Labeled_BPE;
            bool N15Flag = mp->N15Labeled_BPE;
            bool S34Flag = mp->S34Labeled_BPE;
            bool D2Flag = mp->D2Labeled_BPE;

            IsotopeDetection::IsotopeDetectionType isoType;
            isoType = IsotopeDetection::PeakDetection;

            IsotopeDetection isotopeDetection(
                mp,
                isoType,
                C13Flag,
                N15Flag,
                S34Flag,
                D2Flag);
            isotopeDetection.pullIsotopes(group.get());
            for (auto child : group->children)
                child->setTableName(group->tableName());
        }
    }

    mavenParameters->allgroups.clear();
    for (auto group : groups)
        mavenParameters->allgroups.push_back(*(group.get()));
}
