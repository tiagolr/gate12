#include "PatternManager.h"
#include "../dsp/Pattern.h"
#include "../ui/Sequencer.h"
#include "../Globals.h"
#include "../PluginProcessor.h"
#include <sstream>

constexpr int PATTERN_COUNT{ 12 };
void PatternManager::importPatterns(Pattern* patterns[PATTERN_COUNT],const TensionParameters& tensionParameters)
{
	mFileChooser.reset(new juce::FileChooser(importWindowTitle, juce::File(), patternExtension));

	mFileChooser->launchAsync(juce::FileBrowserComponent::openMode |
		juce::FileBrowserComponent::canSelectFiles,
		[this, patterns, tensionParameters](const juce::FileChooser& fc)
		{
			if (fc.getURLResults().size() > 0)
			{
				const auto u = fc.getURLResult();
				auto file = u.getLocalFile();

				if (!file.existsAsFile())
					return;

				juce::String fileContent = file.loadFileAsString();
				std::istringstream iss(fileContent.toStdString());

				for (int i = 0; i < PATTERN_COUNT; ++i)
				{
					patterns[i]->clear();
					patterns[i]->clearUndo();

					double x, y, tension;
					int type;
					std::string line;

					if (!std::getline(iss, line))
						break;

					std::istringstream lineStream(line);
					while (lineStream >> x >> y >> tension >> type)
					{
						patterns[i]->insertPoint(x, y, tension, type, false);
					}
					patterns[i]->setTension(tensionParameters.tension, tensionParameters.tensionAtk, tensionParameters.tensionRel, tensionParameters.dualTension);
					patterns[i]->buildSegments();
				}
			}

			mFileChooser = nullptr;
		}, nullptr);
}

void PatternManager::exportPatterns(Pattern* patterns[PATTERN_COUNT])
{
	mFileChooser.reset(new juce::FileChooser(exportWindowTitle, juce::File::getSpecialLocation(juce::File::commonDocumentsDirectory), patternExtension));

	mFileChooser->launchAsync(juce::FileBrowserComponent::saveMode |
		juce::FileBrowserComponent::canSelectFiles |
		juce::FileBrowserComponent::warnAboutOverwriting, [this, patterns](const juce::FileChooser& fc)
		{
			auto file = fc.getResult();
			if (file == juce::File{})
				return;

			std::ostringstream oss;
			for (int i = 0; i < PATTERN_COUNT; ++i)
			{
				auto points = patterns[i]->points;

				for (const auto& point : points)
				{
					oss << point.x << " " << point.y << " " << point.tension << " " << point.type << " ";
				}
				oss << "\n";
			}

			if (file.replaceWithText(oss.str()))
			{
				auto options = juce::MessageBoxOptions().withIconType(juce::MessageBoxIconType::InfoIcon)
					.withTitle("Export Successful")
					.withMessage("Patterns exported successfully to:\n" + file.getFullPathName())
					.withButton("OK");
				messageBox = juce::NativeMessageBox::showScopedAsync(options, nullptr);
			}
			else
			{
				auto options = juce::MessageBoxOptions().withIconType(juce::MessageBoxIconType::WarningIcon)
					.withTitle("Export Failed")
					.withMessage("Failed to write pattern file:\n" + file.getFullPathName())
					.withButton("OK");
				messageBox = juce::NativeMessageBox::showScopedAsync(options, nullptr);
			}
		});
}
