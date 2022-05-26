#ifndef MATH_ANIM_SEQUENCER_H
#define MATH_ANIM_SEQUENCER_H
#include "core.h"

#include "ImSequencer.h"
#include "ImCurveEdit.h"

namespace MathAnim
{

	struct RampEdit : public ImCurveEdit::Delegate
	{
	public:
		RampEdit();

		bool IsVisible(size_t curveIndex);

		size_t GetCurveCount();
		size_t GetPointCount(size_t curveIndex);
		uint32_t GetCurveColor(size_t curveIndex);
		ImVec2* GetPoints(size_t curveIndex);

		virtual ImCurveEdit::CurveType GetCurveType(size_t curveIndex) const override;
		virtual int EditPoint(size_t curveIndex, int pointIndex, ImVec2 value) override;
		virtual void AddPoint(size_t curveIndex, ImVec2 value) override;
		virtual ImVec2& GetMax() override;
		virtual ImVec2& GetMin() override;
		virtual unsigned int GetBackgroundColor() override;

	public:
		ImVec2 mPts[3][8];
		size_t mPointCount[3];
		bool mbVisible[3];
		ImVec2 mMin;
		ImVec2 mMax;

	private:
		void SortValues(size_t curveIndex);
	};

	struct MathAnimSequencer : ImSequencer::SequenceInterface
	{
	public:
		virtual int GetFrameMin() const override;
		virtual int GetFrameMax() const override;
		virtual int GetItemCount() const override;

		//virtual void BeginEdit(int /*index*/) {}
		//virtual void EndEdit() {}

		virtual int GetItemTypeCount() const;
		virtual const char* GetItemTypeName(int typeIndex) const;
		virtual const char* GetItemLabel(int index) const;
		virtual const char* GetCollapseFmt() const;

		virtual void Get(int index, int** start, int** end, int* type, unsigned int* color) override;
		virtual void Add(int type) override;
		virtual void Del(int index) override;
		virtual void Duplicate(int index) override;

		//virtual void Copy() {}
		//virtual void Paste() {}

		virtual size_t GetCustomHeight(int index) override;
		virtual void DoubleClick(int index) override;

		virtual void CustomDraw(int index, ImDrawList* draw_list, const ImRect& rc, const ImRect& legendRect, const ImRect& clippingRect, const ImRect& legendClippingRect) override;
		virtual void CustomDrawCompact(int index, ImDrawList* draw_list, const ImRect& rc, const ImRect& clippingRect) override;

	public:
		struct MySequenceItem
		{
			int mType;
			int mFrameStart, mFrameEnd;
			bool mExpanded;
		};

	public:
		std::vector<MySequenceItem> myItems;
		int frameMin;
		int frameMax;
		RampEdit rampEdit;
	};
}

#endif 