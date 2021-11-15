/**
 * @class EIC
 * @ingroup libmaven
 * @brief Wrapper class for a eic.
 * @author Sabu George
 */
#ifndef MZEIC_H
#define MZEIC_H

#include <Eigen>

#include "standardincludes.h"
#include "PeakGroup.h"

class Peak;
class PeakGroup;
class mzSample;
class mzPoint;
class Scan;
class Compound;
class mzSlice;
class MavenParameters;

using namespace std;

class EIC
{

  public:
    /**
    *  Default constructor. 
    */
    EIC();

    /**
    *  Destructor
    */
    ~EIC();

    enum SmootherType /**<Enumeration to select the smoothing algorithm */
    {
        SAVGOL = 0,
        GAUSSIAN = 1,
        AVG = 2
    };
    enum EicType /**<Enumeration to select how intensity and/or mass is calculated at a particular retention time */
    {
        MAX = 0,
        SUM = 1
    };

    enum class BaselineMode {
        Threshold,
        AsLSSmoothing
    };

    vector<int> scannum;     /**< Store all scan numbers in an EIC */
    vector<float> rt;        /**< Store all retention times in an EIC */
    vector<float> mz;        /**< Store all mass/charge ratios in an EIC */
    vector<float> intensity; /**< Store all intensities in an EIC */
    vector<Peak> peaks;      /**< Store all peak objects in an EIC */
    string sampleName;       /**< Store name of the sample associated with the EIC */

    mzSample *sample; /**< pointer to originating sample */

    float color[4]; /**< color of the eic line, [r,g,b, alpha] */

    float *spline; /**< pointer to smoothed intensity array */

    float *baseline; /**< pointer to baseline array */

    /* maximum intensity of all scans*/
    float maxIntensity;

    /* rt value of maximum intensity*/
    float rtAtMaxIntensity;

    /* mz value of maximum intensity*/
    float mzAtMaxIntensity;

    float maxAreaTopIntensity; /**< maximum areaTop intensity (after baseline correction) out of all peaks */

    float maxAreaIntensity; /**< maximum area intensity (after baseline correction) out of all peaks */

    float maxAreaNotCorrectedIntensity; /**< maximum area intensity (without baseline correction) out of all peaks */

    float maxAreaTopNotCorrectedIntensity; /**< maximum areaTop intensity (without baseline correction) out of all peaks */

    double filterSignalBaselineDiff; /**< minimum threshold for peak signal-baseline difference */

    float totalIntensity; /**< sum of all intensities in an EIC */

    int eic_noNoiseObs; /**< number of observations above baseline */

    float mzmin; /**< minimum mass/charge for pulling an EIC */
    float mzmax; /**< maximum mass/charge for pulling an EIC */
    float rtmin; /**< minimum retention time for pulling an EIC */
    float rtmax; /**< maximum retention time for pulling an EIC */

    /**
    * @brief add peak object to vector
    * @details create a peak object for given peak position and append to vector
    * @param  peakPos position of peak in spline
    * @return pointer to newly added peak object in the vector
    */
    Peak *addPeak(int peakPos);

    /**
    * @brief delete peak at given index from the vector
    * @param  i index of peak to be deleted
    */
    void deletePeak(unsigned int i);

    /**
     * @brief Keep datapoints only within the given retention time range.
     * @param minRt The lower bound of the desired RT range.
     * @param maxRt The upper bound of the desired RT range.
     */
    void reduceToRtRange(float minRt, float maxRt);

    /**
    * @brief Find peak positions after smoothing, baseline calculation and peak
    * filtering.
    * @param smoothWindow Number of scans used for smoothing in each iteration.
    * @param recomputeBaseline Whether to recompute the baseline. If baseline
    * has not been computed, then it will be anyway.
    */
    void getPeakPositions(int smoothWindow, bool recomputeBaseline = true);

    /**
    * @brief set values for all members of a peak object
    * @param  peak peak object
    */
    void getPeakDetails(Peak &peak);

    /**
    * @brief width of given peak in terms of number of scans
    * @details find the number of scans where peak intensity is above baseline
    * @param  peak peak object
    */
    void getPeakWidth(Peak &peak);

    /**
     * @brief Create a peak spanning over the given RT range in this EIC.
     * @param rtMin The left boundary of the peak.
     * @param rtMax The right boundary of the peak.
     * @return A `Peak` object, which only contains the RT boundary information.
     * A call to `getPeakDetails` might still be needed to make any use of this
     * peak in most contexts.
     */
    Peak peakForRegion(float rtMin, float rtMax);

    void setBaselineMode(BaselineMode b) { _baselineMode = b; }

    /**
     * @brief Calculate baseline for the current baseline mode.
     */
    void computeBaseline();

    /**
    * @brief calculate spline of the EIC
    * @details smoothen intensity data according to selected algorithm. stores it as spline
    * @param  smoothWindow  number of scans used for smoothing in each iteration
    */
    void computeSpline(int smoothWindow);

    /**
    * @brief find the first and last position of a peak
    * @param  peak peak object
    */
    void findPeakBounds(Peak &peak);

    /**
     * @brief Manually adjust the bounds of a single peak, known to this EIC.
     * @details This method should only be used when the user explicitly
     * specifies peak's bounds, with a better judgement than the automatic
     * boundary detection algorithm. No additional checks, such as zero
     * trimming at edges or spline edge detection, will be employed. In fact,
     * peak's spline bounds will be set to be the same as the peak's raw
     * intensity bounds.
     * @param peak The peak to be modified. It should already exist within this
     * EIC object's `peaks` vector.
     * @param rtMin The lower bound on peak's RT span.
     * @param rtMax The upper bound on peak's RT span.
     */
    void adjustPeakBounds(Peak& peak, float rtMin, float rtMax);

    /**
     * @brief Find all parameter values for every peak in an EIC.
     */
    void getPeakStatistics();

    /**
    * @brief find all peaks in an EIC
    * @details find all local maxima in an EIC and save them as objects
    */
    void findPeaks();

    /**
    * @brief remove peaks with parameter values below user-set thresholds
    */
    void filterPeaks();

    /**
    * brief 
    * @param  peak             [peak]
    */
    void checkGaussianFit(Peak &peak);

    /**
    * @brief get vector of all intensity points in a peak
    * @param peak peak object
    * @return mzPoint vector of intensity points in the peak
    */
    vector<mzPoint> getIntensityVector(Peak &peak);

    /**
    * @brief print parameter values of an EIC in log window
    */
    void summary();

    /**
    * @brief set smoothing algorithm
    * @param  x SmootherType 
    */
    void setSmootherType(EIC::SmootherType x) { smootherType = x; }

    /**
    * @brief set smoothing window for baseline
    * @param  x number of scans used for smoothing in one iteration
    */
    void setBaselineSmoothingWindow(int x) { baselineSmoothingWindow = x; }

    /**
    * @brief set percentage of top intensity points to remove for setting baseline
    * @param  x percentage of top intensity points to remove
    */
    void setBaselineDropTopX(int x) { baselineDropTopX = x; }

    /**
     * @brief Set smoothness (λ) to be used for default AsLS baseline estimation.
     * @param s smoothness (will be mutated to 10^s when actually used)
     */
    void setAsLSSmoothness(int s) { _aslsSmoothness = s; }

    /**
     * @brief Set asymmetry (p) to be used for default AsLS baseline estimation.
     * @param a asymmetry value (will be divided by 100 when actually used).
     */
    void setAsLSAsymmetry(int a) { _aslsAsymmetry = a; }

    /**
    * @brief set minimum signal baseline difference for every peak
    * @param x signal baseline difference threshold for every peak
    */
    void setFilterSignalBaselineDiff(double x) { filterSignalBaselineDiff = x; }

    /**
    * @brief get EIC of a sample using given mass/charge and retention time range
    * @details 
    * @param
    * @return bool true if EIC is pulled. false otherwise
    */
    bool makeEICSlice(mzSample *sample,
                      float mzmin,
                      float mzmax,
                      float rtmin,
                      float rtmax,
                      int mslevel,
                      int eicType,
                      string filterline,
                      float precursorMz = -1.0f);

    void getRTMinMaxPerScan();

    void normalizeIntensityPerScan(float scale);

    void subtractBaseLine();
    void clearEICContents();
    void interpolate();
    /**
         * [size ]
         * @method size
         * @return []
         */
    inline unsigned int size() { return intensity.size(); }

    /**
    * @return sample associated with the EIC
    */
    inline mzSample *getSample() { return sample; }

    /**
     * @brief Obtain a list of groups given a set of EICs.
     * @details Assigns every peak to a group based on the best matching merged
     * EIC.
     * @param eics A vector of EICs over which grouping of peaks will take
     * place.
     * @param slice A slice denoting the m/z-rt region of interest. This value
     * will be used to assign a slice to each peak-group.
     * @param A shared pointer to a `MavenParameters` object that will be used
     * to create all new peak-groups for the given EICs.
     * @param An integration type that will be used to tell the method to tag
     * any detected groups with this type.
     * @return A vector of peak-groups found.
    **/
    static vector<PeakGroup>
    groupPeaks(vector<EIC *> &eics,
               mzSlice* slice,
               shared_ptr<MavenParameters> mp,
               PeakGroup::IntegrationType integrationType = PeakGroup::IntegrationType::Programmatic);
    /**
         * [eicMerge ]
         * @method eicMerge
         * @param  eics     []
         * @return []
         */
    static EIC *eicMerge(const vector<EIC *> &eics);

    /**
         * [remove Low Rank Groups ]
         * @method removeLowRankGroups
         * @param  groups              [vector of peak groups]
         * @param  rankLimit           [group rank limit ]
         */
    static void removeLowRankGroups(vector<PeakGroup> &groups, unsigned int rankLimit);

    /**
         * [compare Max Intensity]
         * @method compMaxIntensity
         * @param  a                [EIC a]
         * @param  b                [EIC b]
         * @return [true or false]
         */
    static bool compMaxIntensity(EIC *a, EIC *b) { return a->maxIntensity > b->maxIntensity; }

  private:
    /**
     * Name of selected smoothing algorithm
     */
    SmootherType smootherType;

    /**
     * @brief _baselineMode decides which algorithm to use for computing baseline.
     */
    BaselineMode _baselineMode;

    /**
     * Sets the number of scans used for smoothing in one iteration.
     */
    int baselineSmoothingWindow;

    /*
     * Percentage of top intensity points to remove before computing baseline.
     */
    int baselineDropTopX;

    /**
     * @brief Smoothness parameter for AsLS Smoothing algorithm
     */
    int _aslsSmoothness;

    /**
     * @brief Asymmetry parameter for AsLS Smoothing algorithm
     */
    int _aslsAsymmetry;

    /**
     * @brief Clear the baseline if exists and reallocate memory for a new one.
     * @return Whether the baseline should be processed further or not.
     */
    bool _clearBaseline();

    /**
     * @brief Computes a baseline using naive thresholding method.
     * @param smoothingWindow is the size of window used for 1D guassian smoothing.
     * @param dropTopX percent of the highest intensities will be truncated.
     */
    void _computeThresholdBaseline(const int smoothingWindow,
                                   const int dropTopX);

    /**
     * @brief Computes a baseline using Asymmetric Least Squares Smoothing techinique.
     * @details A (Whittaker) smoother is used to get a slowly varying estimate
     * of the baseline. In contrast to ordinary least squares smoothing,
     * however, positive deviations with respect to baseline estimate are
     * weighted (much) less than negative ones.
     *
     * Ref: Baseline Correction with Asymmetric Least Squares Smoothing,
     * P. Eilers, H. Boelens, 2005
     *
     * @param lambda for smoothness. Typical values of lambda for MS data range
     * from 10^2 to 10^9, depending on dataset. But since we resample the
     * intensity signal, we can limit this range to [10^0, 10^3]. The exponent value
     * should be passed here as integer, i.e. lambda should be in range [0, 3].
     * @param p for asymmetry. Values between 0.01 to 0.10 work reasonable well
     * for MS data.
     * @param numIterations for the number of iterations that should be
     * performed (since this is an iterative optimization algorithm).
     */
    void _computeAsLSBaseline(const float lambda,
                              const float p,
                              const int numIterations=10);
};
#endif //MZEIC_H
