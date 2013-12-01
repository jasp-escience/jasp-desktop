#include "analysisloader.h"

#include "analyses/descriptives.h"
#include "analyses/ttestonesample.h"
#include "analyses/ttestindependentsamples.h"
#include "analyses/ttestbayesianonesample.h"
#include "analyses/ttestpairedsamples.h"
#include "analyses/anovaoneway.h"
#include "analyses/anovamultivariate.h"
#include "analyses/anova.h"
#include "analyses/ancova.h"
#include "analyses/anovabayesian.h"
#include "analyses/ancovamultivariate.h"
#include "analyses/regressionlinear.h"
#include "analyses/contingencytables.h"
#include "analyses/correlation.h"

#include "analysispart.h"

Analysis *AnalysisLoader::load(int id, string analysisName)
{

	if (analysisName == "Descriptives")
		return new Descriptives(id);
	else if (analysisName == "TTestOneSample")
		return new TTestOneSample(id);
	else if (analysisName == "TTestIndependentSamples")
		return new TTestIndependentSamples(id);
	else if (analysisName == "TTestPairedSamples")
		return new TTestPairedSamples(id);
	else if (analysisName == "TTestBayesianOneSample")
		return new TTestBayesianOneSample(id);
	else if (analysisName == "AnovaOneWay")
		return new AnovaOneWay(id);
	else if (analysisName == "AnovaMultivariate")
		return new AnovaMultivariate(id);
	else if (analysisName == "AncovaMultivariate")
		return new AncovaMultivariate(id);
	else if (analysisName == "Anova")
		return new Anova(id);
	else if (analysisName == "Ancova")
		return new Ancova(id);
	else if (analysisName == "AnovaBayesian")
		return new AnovaBayesian(id);
	else if (analysisName == "RegressionLinear")
		return new RegressionLinear(id);
	else if (analysisName == "ContingencyTables")
		return new ContingencyTables(id);
	else if (analysisName == "Correlation")
		return new Correlation(id);
	else
	{
		std::cout << "AnalysisLoader::laod(); Analysis not found: " << analysisName << "\n";
		std::cout.flush();

		return NULL;
	}

}
