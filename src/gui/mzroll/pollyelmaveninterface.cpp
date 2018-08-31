#include "pollyelmaveninterface.h"
#include <string>
#include <QVariant>

PollyElmavenInterfaceDialog::PollyElmavenInterfaceDialog(MainWindow* mw) :
        QDialog(mw),
        mainwindow(mw),
        _loginform(NULL)
{
        setModal(true);
        setupUi(this);
        createIcons();
        workflowMenu->setCurrentRow(0);

        _pollyIntegration = new PollyIntegration();
        _loadingDialog = new PollyWaitDialog(this);
        pollyButton->setVisible(false);
        fluxButton->setVisible(false);
        
        connect(pollyButton, SIGNAL(clicked(bool)), SLOT(goToPolly()));
        connect(fluxButton, SIGNAL(clicked(bool)), SLOT(goToPolly()));
        //connect(checkBox_advanced_settings,SIGNAL(clicked(bool)),SLOT(showAdvanceSettings()));
        connect(firstViewUpload, SIGNAL(clicked(bool)), SLOT(uploadDataToPolly()));
        connect(fluxUpload, SIGNAL(clicked(bool)), SLOT(uploadDataToPolly()));
        connect(cancelButton_upload, SIGNAL(clicked(bool)), SLOT(cancel()));
        connect(cancelButtonFlux, SIGNAL(clicked(bool)), SLOT(cancel()));
        connect(new_project_radio_button, SIGNAL(clicked(bool)), SLOT(handle_new_project_radio_button()));
        connect(radioNewProject_flux, SIGNAL(clicked(bool)), SLOT(handle_new_project_radio_button()));
        connect(existing_project_radio_button, SIGNAL(clicked(bool)), SLOT(handle_existing_project_radio_button()));
        connect(radioSelectProject_flux, SIGNAL(clicked(bool)), SLOT(handle_existing_project_radio_button()));
        connect(logout_upload, SIGNAL(clicked(bool)), SLOT(logout()));
        connect(logout_upload1, SIGNAL(clicked(bool)), SLOT(logout()));
        connect(this, SIGNAL(uploadFinished(bool)), SLOT(performPostUploadTasks(bool)));
}
 
PollyElmavenInterfaceDialog::~PollyElmavenInterfaceDialog()
{
    qDebug()<<"exiting PollyElmavenInterfaceDialog now....";
    if (_loginform) delete (_loginform);
}

EPIWorkerThread::EPIWorkerThread()
{
    _pollyintegration = new PollyIntegration();   
    
};

void EPIWorkerThread::run(){
    if (state=="initial_setup"){
        qDebug()<<"starting thread to authenticate and fetch project info from polly";
        QString status_inside = _pollyintegration->authenticate_login("","");
        emit authentication_result(status_inside);
        if (status_inside=="ok"){
            QVariantMap projectnames_id = _pollyintegration->getUserProjects();
            emit resultReady(projectnames_id);
        }
    }
    else if (state=="upload_files"){
        qDebug()<<"starting thread for uploading files to polly..";
        //re-login to polly may be required because the token expires after 30 minutes..
        QString status_inside = _pollyintegration->authenticate_login("","");
        QStringList patch_ids  = _pollyintegration->exportData(filesToUpload,upload_project_id_thread);
        bool status = tmpDir.removeRecursively();
        emit filesUploaded(patch_ids, upload_project_id_thread, datetimestamp);
    }
}

EPIWorkerThread::~EPIWorkerThread()
{
    if (_pollyintegration) delete (_pollyintegration);
};

void PollyElmavenInterfaceDialog::createIcons()
{   
    QListWidgetItem *firstView = new QListWidgetItem(workflowMenu);
    firstView->setIcon(QIcon(":/images/firstView.png"));
    firstView->setText(tr("FirstView"));
    firstView->setTextAlignment(Qt::AlignHCenter);
    firstView->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);
    firstView->setToolTip("FirstView: Preview El-MAVEN processed data and perform further analysis");

    QListWidgetItem *fluxomics = new QListWidgetItem(workflowMenu);
    fluxomics->setIcon(QIcon(":/images/flux.png"));
    fluxomics->setText(tr("PollyPhi Relative"));
    fluxomics->setTextAlignment(Qt::AlignHCenter);
    fluxomics->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);
    fluxomics->setToolTip("PollyPhi Relative: Process and derive insights from Relative flux workflow");

    workflowMenu->setSizeAdjustPolicy(QListWidget::AdjustToContents);
    workflowMenu->setViewMode(QListView::IconMode);
    workflowMenu->setFlow(QListView::TopToBottom);
    workflowMenu->setSpacing(18);
    workflowMenu->setIconSize(QSize(140, 140));

    connect(workflowMenu, SIGNAL(currentItemChanged(QListWidgetItem*, QListWidgetItem*)),
        this, SLOT(changePage(QListWidgetItem*, QListWidgetItem*)));
}

void PollyElmavenInterfaceDialog::changePage(QListWidgetItem *current, QListWidgetItem *previous)
 {
    if (!current)
        current = previous;

    stackedWidget->setCurrentIndex(workflowMenu->row(current));
 }

void PollyElmavenInterfaceDialog::goToPolly()
{
    QDesktopServices::openUrl(pollyURL);
}

void PollyElmavenInterfaceDialog::handle_new_project_radio_button(){
    if (stackedWidget->currentIndex() == 0) {
        lineEdit_new_project_name->setEnabled(true);
        comboBox_existing_projects->setEnabled(false);
    }
    else {
        newProjectName_flux->setEnabled(true);
        projectList_flux->setEnabled(false);
    }
    QCoreApplication::processEvents();
}

void PollyElmavenInterfaceDialog::handle_existing_project_radio_button(){
    if (stackedWidget->currentIndex() == 0) {
        lineEdit_new_project_name->setEnabled(false);
        comboBox_existing_projects->setEnabled(true);
    }
    else {
        newProjectName_flux->setEnabled(false);
        projectList_flux->setEnabled(true);
    }
    QCoreApplication::processEvents();
}

void PollyElmavenInterfaceDialog::showAdvanceSettings(){
    use_advanced_settings = "yes";
    _advancedSettings = new AdvancedSettings();
    _advancedSettings->initialSetup();
}

void PollyElmavenInterfaceDialog::initialSetup()
{   
    int node_status = _pollyIntegration->check_node_executable();
    if (node_status==0){
        QMessageBox msgBox(this);
        msgBox.setWindowModality(Qt::NonModal);
        msgBox.setWindowTitle("node is not installed on this system");
        msgBox.exec();
        return;
    }
    int askForLogin = _pollyIntegration->askForLogin();
    if (askForLogin==1){
        try{
            call_login_form();
            } catch(...) {
                QMessageBox msgBox(this);
                msgBox.setWindowModality(Qt::NonModal);
                msgBox.setWindowTitle("couldn't load login form");
                msgBox.exec();
            }
        return;
    }
    else{
        try{
            call_initial_EPI_form();
            } catch(...) {
                QMessageBox msgBox(this);
                msgBox.setWindowModality(Qt::NonModal);
                msgBox.setWindowTitle("couldn't load initial form");
                msgBox.exec();
            }
        return;
    }
}

void PollyElmavenInterfaceDialog::call_login_form(){
    _loginform =new LoginForm(this);
    _loginform->setModal(true);
    _loginform->show();
}

void PollyElmavenInterfaceDialog::call_initial_EPI_form(){
    if (stackedWidget->currentIndex() == 0) {
        firstViewUpload->setEnabled(false);
        comboBox_existing_projects->clear();
    }
    else {
        fluxUpload->setEnabled(false);
        projectList_flux->clear();
    }
    
    EPIWorkerThread *EPIworkerThread = new EPIWorkerThread();
    connect(EPIworkerThread, SIGNAL(resultReady(QVariantMap)), this, SLOT(handleResults(QVariantMap)));
    connect(EPIworkerThread, SIGNAL(authentication_result(QString)), this, SLOT(handleAuthentication(QString)));
    connect(EPIworkerThread, &EPIWorkerThread::finished, EPIworkerThread, &QObject::deleteLater);
    EPIworkerThread->state = "initial_setup";
    EPIworkerThread->start();
    show();
    
    _loadingDialog->setModal(true);
    _loadingDialog->show();
    _loadingDialog->statusLabel->setVisible(true);
    _loadingDialog->statusLabel->setText("Authenticating..");
    _loadingDialog->label->setVisible(true);
    _loadingDialog->label->setMovie(_loadingDialog->movie);
    _loadingDialog->label->setAlignment(Qt::AlignCenter);
    QCoreApplication::processEvents();
}

void PollyElmavenInterfaceDialog::handleAuthentication(QString status){
    if (status=="ok"){
        _loadingDialog->statusLabel->setText("Fetching your projects..");
        QCoreApplication::processEvents();
    }
    else {
            _loadingDialog->statusLabel->setText("Authentication failed. Please login again.");
            QCoreApplication::processEvents();
            logout();
    }
}

void PollyElmavenInterfaceDialog::handleResults(QVariantMap projectnames_id_map){
    projectnames_id = projectnames_id_map;
    startup_data_load();
}

void PollyElmavenInterfaceDialog::setFluxPage()
{
    //index 1 corresponds to Fluxomics
    //TODO: use an enum to make this less error prone and more scalable
    stackedWidget->setCurrentIndex(1);
    workflowMenu->setCurrentRow(1);
}

void PollyElmavenInterfaceDialog::setUiElementsFlux()
{
    tableList_flux->clear();
    radioNewProject_flux->setChecked(true);
    newProjectName_flux->setEnabled(true);
    
    radioSelectProject_flux->setChecked(false);
    projectList_flux->setEnabled(false);
    projectList_flux->clear();
    
    fluxButton->setVisible(false);
    fluxUpload->setEnabled(true);
}

void PollyElmavenInterfaceDialog::setUiElementsFV()
{
    comboBox_table_name->clear();

    new_project_radio_button->setChecked(true);
    lineEdit_new_project_name->setEnabled(true);
    
    existing_project_radio_button->setChecked(false);
    comboBox_existing_projects->setEnabled(false);
    comboBox_existing_projects->clear();
    
    pollyButton->setVisible(false);
    firstViewUpload->setEnabled(true);
}

QVariantMap PollyElmavenInterfaceDialog::startup_data_load()
{
    setUiElementsFlux();
    setUiElementsFV();

    QCoreApplication::processEvents();

    QIcon project_icon(rsrcPath + "/POLLY.png");
    if (projectnames_id.isEmpty()) {
        projectnames_id = _pollyIntegration->getUserProjects();
    }    
    QStringList keys= projectnames_id.keys();
    for (int i = 0; i < keys.size(); ++i){
        projectList_flux->addItem(project_icon, projectnames_id[keys.at(i)].toString());
        comboBox_existing_projects->addItem(project_icon, projectnames_id[keys.at(i)].toString());
    }

    QList<QPointer<TableDockWidget> > peaksTableList = mainwindow->getPeakTableList();
    bookmarkedPeaks = mainwindow->getBookmarkedPeaks();
    if (!bookmarkedPeaks->getGroups().isEmpty()) {
        bookmarkTableNameMapping[QString("Bookmark Table")] = bookmarkedPeaks;
        tableList_flux->addItem("Bookmark Table");
        comboBox_table_name->addItem("Bookmark Table");
    } 

    int n = peaksTableList.size();
    if (n > 0) {
        for (int i = 0; i < n; ++i){
            QString peak_table_name = QString("Peak Table ")+QString::number(i+1);
            peakTableNameMapping[peak_table_name] = peaksTableList.at(i);
            tableList_flux->addItem(peak_table_name);
            comboBox_table_name->addItem(peak_table_name);
        }        
    }

    _loadingDialog->close();
    QCoreApplication::processEvents();
    
    return projectnames_id;
}

void PollyElmavenInterfaceDialog::uploadDataToPolly()
{
    fluxUpload->setEnabled(false);
    firstViewUpload->setEnabled(false);
    int askForLogin = _pollyIntegration->askForLogin();
    if (askForLogin == 1) {
        call_login_form();
        emit uploadFinished(false);
        return;
    }
    
    QStringList patch_ids;

    QString new_projectname;
    QString projectname;
    if (stackedWidget->currentIndex() == 0) {
        new_projectname = lineEdit_new_project_name->text();
        projectname = comboBox_existing_projects->currentText();
    }
    else {
        new_projectname = newProjectName_flux->text();
        projectname = projectList_flux->currentText();
    }
    
    QString project_id;

    QString writable_temp_dir =  QStandardPaths::writableLocation(QStandardPaths::QStandardPaths::GenericConfigLocation) + QDir::separator() + "tmp_Elmaven_Polly_files";
    qDebug()<<"writing Polly temp file in this directory -"<<writable_temp_dir;
    QDir qdir(writable_temp_dir);
    if (!qdir.exists()){
        QDir().mkdir(writable_temp_dir);
        QDir qdir(writable_temp_dir);
    }
    
    if (stackedWidget->currentIndex() == 0) {
        upload_status->setStyleSheet("QLabel {color : green; }");
        upload_status->setText("Preparing files..");
    }
    else {
        fluxStatus->setStyleSheet("QLabel {color : green; }");
        fluxStatus->setText("Preparing files..");
    }
    QCoreApplication::processEvents();
    QDateTime current_time;
    QString datetimestamp = current_time.currentDateTime().toString();
    datetimestamp.replace(" ","_");
    datetimestamp.replace(":","-");
    
    QStringList filenames = prepareFilesToUpload(qdir, datetimestamp);
    if (filenames.isEmpty()) {
        if (stackedWidget->currentIndex() == 0)
            upload_status->setText("File preparation failed.");
        else    fluxStatus->setText("File preparation failed.");
        _loadingDialog->close();
        QCoreApplication::processEvents();
        emit uploadFinished(false);
        return;
    }
    if (stackedWidget->currentIndex() == 0)
        upload_status->setText("Connecting..");
    else fluxStatus->setText("Connecting..");
    QCoreApplication::processEvents();
    //re-login to polly may be required because the token expires after 30 minutes..
    QString status_inside = _pollyIntegration->authenticate_login(credentials.at(0),credentials.at(1));
    if (stackedWidget->currentIndex() == 0)
        upload_status->setText("Sending files to Polly..");
    else fluxStatus->setText("Sending files to Polly..");
    
    QCoreApplication::processEvents();
    
    if ((stackedWidget->currentIndex() == 0 && comboBox_existing_projects->isEnabled()) 
            || (stackedWidget->currentIndex() == 1 && projectList_flux->isEnabled())) {
        QStringList keys = projectnames_id.keys();
        for (int i = 0; i < keys.size(); ++i) {
            if (projectnames_id[keys.at(i)].toString() == projectname) {
                project_id = keys.at(i);
            }
        }
        if (project_id != "") {
            upload_project_id = project_id;
        }
        else {
            QString msg = "No such project on Polly..";
            QMessageBox msgBox(mainwindow);
            msgBox.setWindowTitle("Error");
            msgBox.setText(msg);
            msgBox.exec();
        }
    }
    else if ((stackedWidget->currentIndex() == 0 && lineEdit_new_project_name->isEnabled()) 
            || (stackedWidget->currentIndex() == 1 && newProjectName_flux->isEnabled())) {
        if (new_projectname == "") {
            QString msg = "Invalid Project name";
            QMessageBox msgBox(mainwindow);
            msgBox.setWindowTitle("Error");
            msgBox.setText(msg);
            msgBox.exec();
            upload_status->setText("");
            fluxStatus->setText("");
            emit uploadFinished(false);
            return;
        }
        QString new_project_id = _pollyIntegration->createProjectOnPolly(new_projectname);
        upload_project_id = new_project_id;    
    }
    else {
        QString msg = "Please select at least one option";
        QMessageBox msgBox(mainwindow);
        msgBox.setWindowTitle("Error");
        msgBox.setText(msg);
        msgBox.exec();
        upload_status->setText("");
        fluxStatus->setText("");
        emit uploadFinished(false);
        return;
    }
    
    EPIWorkerThread *EPIworkerThread = new EPIWorkerThread();
    connect(EPIworkerThread, SIGNAL(filesUploaded(QStringList, QString, QString)), this, SLOT(postUpload(QStringList, QString, QString)));
    connect(EPIworkerThread, &EPIWorkerThread::finished, EPIworkerThread, &QObject::deleteLater);
    EPIworkerThread->state = "upload_files";
    EPIworkerThread->filesToUpload = filenames;
    EPIworkerThread->tmpDir = qdir;
    EPIworkerThread->upload_project_id_thread = upload_project_id;
    EPIworkerThread->datetimestamp = datetimestamp;
    EPIworkerThread->start();
}

void PollyElmavenInterfaceDialog::postUpload(QStringList patch_ids, QString upload_project_id_thread, QString datetimestamp){
    QCoreApplication::processEvents();
    
    if (!patch_ids.isEmpty()) {
        upload_status->setText("");
        QString redirection_url = QString("https://polly.elucidata.io/main#project=%1&auto-redirect=firstview&elmavenTimestamp=%2").arg(upload_project_id_thread).arg(datetimestamp);
        fluxStatus->setText("");
        qDebug() << "redirection_url     - " << redirection_url;
        pollyURL.setUrl(redirection_url);
        if (stackedWidget->currentIndex() == 0)
            pollyButton->setVisible(true);
        else
            fluxButton->setVisible(true);
    }
    else {
        if (stackedWidget->currentIndex() == 0) {
            upload_status->setStyleSheet("QLabel {color : red; }");
            upload_status->setText("Error!");
        }
        else {
            fluxStatus->setStyleSheet("QLabel {color: red; }");
            fluxStatus->setText("Error!");
        }
        QString msg = "Unable to send data";
        QMessageBox msgBox(mainwindow);
        msgBox.setWindowTitle("Warning!!");
        msgBox.setText(msg);
        msgBox.exec();
        upload_status->setText("");
        fluxStatus->setText("");
    }
    emit uploadFinished(false);
}

QString PollyElmavenInterfaceDialog::getRedirectionUrl(QString dirPath, QString datetimestamp, QString upload_project_id) {
    if (stackedWidget->currentIndex() == 1) { 
        redirectTo = "relative_lcms_elmaven";
        QString CohortFileName = dirPath + QDir::separator() + datetimestamp + "_Cohort_Mapping_Elmaven.csv";
        if (!_pollyIntegration->validSampleCohort(CohortFileName))
            redirectTo = "gsheet_sym_polly_elmaven";
    }    
    QString redirection_url = QString("https://polly.elucidata.io/main#project=%1&auto-redirect=%2&elmavenTimestamp=%3").arg(upload_project_id).arg(redirectTo).arg(datetimestamp);
    return redirection_url;
}

QStringList PollyElmavenInterfaceDialog::prepareFilesToUpload(QDir qdir, QString datetimestamp) {
    
    QString writable_temp_dir =  QStandardPaths::writableLocation(QStandardPaths::QStandardPaths::GenericConfigLocation) + QDir::separator() + "tmp_Elmaven_Polly_files";
    QString peak_table_name;
    if (stackedWidget->currentIndex() == 0)
        peak_table_name = comboBox_table_name->currentText();
    else peak_table_name = tableList_flux->currentText();
    
    QStringList filenames;
    if (peak_table_name != "") {
        if (peak_table_name == "Bookmark Table") {
            _tableDockWidget = bookmarkedPeaks;
        }
        else{
            _tableDockWidget = peakTableNameMapping[peak_table_name];
        }

        if (_tableDockWidget->groupCount() == 0){
            QString msg = "Peaks Table is Empty";
            QMessageBox msgBox(mainwindow);
            msgBox.setWindowTitle("Error");
            msgBox.setText(msg);
            msgBox.exec();
            return filenames;
        }
    }
    else{
        QString msg = "No Peak tables";
        QMessageBox msgBox(mainwindow);
        msgBox.setWindowTitle("Error");
        msgBox.setText(msg);
        msgBox.exec();
        return filenames;
    }

    if (stackedWidget->currentIndex() == 0) {
        upload_status->setStyleSheet("QLabel {color : green; }");
        upload_status->setText("Preparing compound database file..");
    }
    else {
        fluxStatus->setStyleSheet("QLabel {color : green; }");
        fluxStatus->setText("Preparing compound database file..");
    }
    
    QCoreApplication::processEvents();

    if (use_advanced_settings=="yes"){
        handle_advanced_settings(writable_temp_dir,datetimestamp);
    }
    
    if (use_advanced_settings != "yes" || !_advancedSettings->get_upload_compoundDB()) {
        // Now uploading the Compound DB that was used for peak detection.
        // This is needed for Elmaven->Firstview->PollyPhi relative LCMS workflow.
        // ToDo Kailash, Keep track of compound DB used for each peak table,
        // As of now, uploading what is currently there in the compound section of Elmaven.
        // If advanced settings were used to upload compound DB, we need not do this.
        QString compound_db = mainwindow->ligandWidget->getDatabaseName();
        mainwindow->ligandWidget->saveCompoundList(writable_temp_dir+QDir::separator()+datetimestamp+"_Compound_DB_Elmaven.csv",compound_db);
    }

    qDebug()<< "Now uploading all groups, needed for firstview app..";
    _tableDockWidget->wholePeakSet();
    _tableDockWidget->treeWidget->selectAll();
    _tableDockWidget->prepareDataForPolly(writable_temp_dir,"Groups Summary Matrix Format Comma Delimited (*.csv)",datetimestamp+"_Peak_table_all");
    
    //Preparing the json file -
    if (stackedWidget->currentIndex() == 0) {
        upload_status->setStyleSheet("QLabel {color : green; }");
        upload_status->setText("Preparing json file..");
    }
    else {
        fluxStatus->setStyleSheet("QLabel {color : green; }");
        fluxStatus->setText("Preparing json file..");
    }
    QCoreApplication::processEvents();

    //Preparing the json file 
    QString json_filename = writable_temp_dir+QDir::separator()+datetimestamp+"_Peaks_information_json_Elmaven_Polly.json";//  uploading the json file
    _tableDockWidget->exportJsonToPolly(writable_temp_dir,json_filename);
    
    if (stackedWidget->currentIndex() == 1) {
        fluxStatus->setStyleSheet("QLabel {color : green; }");
        fluxStatus->setText("Preparing sample cohort file..");
        QCoreApplication::processEvents();
        //Preparing the sample cohort file
        QString sampleCohortFileName = writable_temp_dir + QDir::separator() + datetimestamp +
                                        "_Cohort_Mapping_Elmaven.csv";
        mainwindow->projectDockWidget->prepareSampleCohortFile(sampleCohortFileName);
    }
    
    //Saving settings file
    QByteArray ba = (writable_temp_dir + QDir::separator() + datetimestamp + "_maven_analysis_settings" + ".xml").toLatin1();
    const char *save_path = ba.data();
    mainwindow->mavenParameters->saveSettings(save_path);
    qDebug() << "settings saved now";
    
    qdir.setFilter(QDir::Files | QDir::NoSymLinks);
    QFileInfoList file_list = qdir.entryInfoList();
    
    for (int i = 0; i < file_list.size(); ++i){
        QFileInfo fileInfo = file_list.at(i);
        QString tmp_filename = writable_temp_dir+QDir::separator()+fileInfo.fileName();
        filenames.append(tmp_filename);
    }
    return filenames;
}

void PollyElmavenInterfaceDialog::handle_advanced_settings(QString writable_temp_dir,QString datetimestamp){
        QVariantMap advanced_ui_elements = _advancedSettings->get_ui_elements();

        QString export_option = advanced_ui_elements["export_option"].toString();
        QString export_format = advanced_ui_elements["export_format"].toString();
        QString user_filename = advanced_ui_elements["user_filename"].toString();
        QString compound_db = advanced_ui_elements["compound_db"].toString();
        QString user_compound_DB_name = advanced_ui_elements["user_compound_DB_name"].toString();
        
        if (user_filename==""){
            user_filename = "intensity_file.csv";
        }
        if (user_compound_DB_name==""){
            user_compound_DB_name = "compounds";
        }
        if (_advancedSettings->get_upload_compoundDB()){
            mainwindow->ligandWidget->saveCompoundList(writable_temp_dir+QDir::separator()+datetimestamp+"_Compound_DB_"+user_compound_DB_name+".csv",compound_db);
        }
        
        if (_advancedSettings->get_upload_Peak_Table()){
            if (export_option=="Export Selected"){
                _tableDockWidget->selectedPeakSet();
            }
            else if(export_option=="Export Good"){
                _tableDockWidget->goodPeakSet();
                _tableDockWidget->treeWidget->selectAll();
            }
            else if(export_option=="Export All Groups"){
                _tableDockWidget->wholePeakSet();
                _tableDockWidget->treeWidget->selectAll();
            }
            else if(export_option=="Export Bad"){
                _tableDockWidget->badPeakSet();
                _tableDockWidget->treeWidget->selectAll();
            }
            _loadingDialog->statusLabel->setStyleSheet("QLabel {color : green; }");
            _loadingDialog->statusLabel->setText("Preparing intensity file..");
            QCoreApplication::processEvents();

            QString peak_user_filename  = datetimestamp+"_Peak_table_" +user_filename;
            qDebug()<<"peak_user_filename - "<<peak_user_filename;
            _tableDockWidget->prepareDataForPolly(writable_temp_dir,export_format,peak_user_filename);
        }

}

void PollyElmavenInterfaceDialog::cancel() {
    use_advanced_settings = "no";
    close();   
}

void PollyElmavenInterfaceDialog::logout() {
    use_advanced_settings = "no";
    _pollyIntegration->logout();
    credentials = QStringList();
    projectnames_id = QVariantMap();
    close();   
}

void PollyElmavenInterfaceDialog::performPostUploadTasks(bool uploadSuccessful) {
    firstViewUpload->setEnabled(true);
    fluxUpload->setEnabled(true);
}