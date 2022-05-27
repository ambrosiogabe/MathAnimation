#include "core.h"

#include "editor/Timeline.h"
#include "editor/MathAnimSequencer.h"
#include "editor/ImGuiTimeline.h"
#include "core/Application.h"

#include "imgui.h"
#define IMGUI_DEFINE_MATH_OPERATORS
#include "imgui_internal.h"
#include "ImGuizmo.h"
#include "ImSequencer.h"
#include "ImCurveEdit.h"

namespace MathAnim
{
	namespace Timeline
	{
		// ------- Private variables --------
		static MathAnimSequencer sequencer;

		static const char* SequencerItemTypeNames[] =
		{
			"Camera",
			"Music",
			"ScreenEffect",
			"FadeIn",
			"Animation"
		};

		void init()
		{
			sequencer.frameMin = -100;
			sequencer.frameMax = 1000;
			sequencer.myItems.push_back(MathAnimSequencer::MySequenceItem{ 0, 10, 30, false });
			sequencer.myItems.push_back(MathAnimSequencer::MySequenceItem{ 1, 20, 30, true });
			sequencer.myItems.push_back(MathAnimSequencer::MySequenceItem{ 3, 12, 60, false });
			sequencer.myItems.push_back(MathAnimSequencer::MySequenceItem{ 2, 61, 90, false });
			sequencer.myItems.push_back(MathAnimSequencer::MySequenceItem{ 4, 90, 99, false });
		}

		void update()
		{
			ImGui::Begin("Timeline");

			static int selectedEntry = -1;
			static int firstFrame = 0;
			static bool expanded = true;
			int currentFrame = Application::getFrameIndex();

			ImGui::PushItemWidth(130);
			ImGui::InputInt("Frame Min", &sequencer.frameMin);
			ImGui::SameLine();
			ImGui::InputInt("Frame ", &currentFrame);
			ImGui::SameLine();
			ImGui::InputInt("Frame Max", &sequencer.frameMax);
			ImGui::PopItemWidth();
			//ImSequencer::Sequencer(&sequencer, &currentFrame, &expanded, &selectedEntry, &firstFrame, ImSequencer::SEQUENCER_EDIT_STARTEND | ImSequencer::SEQUENCER_ADD | ImSequencer::SEQUENCER_DEL | ImSequencer::SEQUENCER_COPYPASTE | ImSequencer::SEQUENCER_CHANGE_FRAME);
			if (ImGuiTimeline(nullptr, 0, &currentFrame, &firstFrame, 12 * 60))
			{
				Application::setFrameIndex(currentFrame);
			}

			if (selectedEntry != -1)
			{
				const MathAnimSequencer::MySequenceItem& item = sequencer.myItems[selectedEntry];
				ImGui::Text("I am a %s, please edit me", SequencerItemTypeNames[item.mType]);
				// switch (type) ....
			}

			ImGui::End();
		}

		void free()
		{

		}
	}
}