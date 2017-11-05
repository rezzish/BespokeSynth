//
//  RadioButton.cpp
//  modularSynth
//
//  Created by Ryan Challinor on 12/5/12.
//
//

#include "RadioButton.h"
#include "IDrawableModule.h"
#include "SynthGlobals.h"
#include "FileStream.h"

const int radioSpacing = 15;

//static
int RadioButton::GetSpacing()
{
   return radioSpacing;
}

RadioButton::RadioButton(IRadioButtonListener* owner, const char* name, int x, int y, int* var, RadioDirection direction /*= kRadioVertical*/)
: mWidth(15)
, mHeight(15)
, mVar(var)
, mOwner(owner)
, mMultiSelect(false)
, mDirection(direction)
, mElementWidth(8)
, mSliderVal(0)
{
   assert(owner);
   SetName(name);
   SetPosition(x,y);
   SetParent(dynamic_cast<IClickable*>(owner));
   
   (dynamic_cast<IDrawableModule*>(owner))->AddUIControl(this);
}

RadioButton::~RadioButton()
{
}

void RadioButton::AddLabel(const char* label, int value)
{
   RadioButtonElement element;
   element.mLabel = label;
   element.mValue = value;
   mElements.push_back(element);
   
   UpdateDimensions();
   
   CalcSliderVal();
}

void RadioButton::SetLabel(const char* label, int value)
{
   for (int i=0; i<mElements.size(); ++i)
   {
      if (mElements[i].mValue == value)
         mElements[i].mLabel = label;
   }
   
   UpdateDimensions();
}

void RadioButton::UpdateDimensions()
{
   if (mDirection == kRadioVertical)
   {
      for (int i=0; i<mElements.size(); ++i)
      {
         int width = GetStringWidth(mElements[i].mLabel) + 5;
         if (width > mWidth)
            mWidth = width;
      }
      mHeight = radioSpacing * (int)mElements.size();
      mElementWidth = mWidth;
   }
   else
   {
      for (int i=0; i<mElements.size(); ++i)
      {
         int width = GetStringWidth(mElements[i].mLabel) + 5;
         if (width > mElementWidth)
            mElementWidth = width;
      }
      mWidth = mElementWidth * (int)mElements.size();
      mHeight = radioSpacing;
   }
}

void RadioButton::Clear()
{
   mElements.clear();
   mWidth = 40;
   mHeight = 15;
}

void RadioButton::Poll()
{
   if (*mVar != mLastSetValue)
      CalcSliderVal();
}

void RadioButton::Render()
{
   ofPushStyle();
   
   DrawBeacon(mX+mWidth/2, mY+mHeight/2);

   int w,h;
   GetDimensions(w,h);
   ofFill();
   ofSetColor(0, 0, 0, gModuleDrawAlpha * .5f);
   ofRect(mX+1,mY+1,mWidth,mHeight);
   for (int i=0; i<mElements.size(); ++i)
   {
      ofColor color,textColor;
      IUIControl::GetColors(color, textColor);
      
      bool active = false;
      if (mVar)
      {
         if (mMultiSelect)
            active = (1 << mElements[i].mValue) & *mVar;
         else
            active = mElements[i].mValue == *mVar;
      }
      
      if (active)
      {
         float h,s,b;
         color.getHsb(h, s, b);
         color.setHsb(42, s, b);
         textColor.set(255,255,0,gModuleDrawAlpha);
      }

      ofFill();
      if (active)
         color.setBrightness(ofLerp(color.getBrightness(), 255, .3f));
      ofSetColor(color);
      
      float x,y;
      
      if (mDirection == kRadioVertical)
      {
         x = mX;
         y = mY+i*radioSpacing;
         ofRect(x,y,w,radioSpacing);
      }
      else
      {
         x = mX + mElementWidth * i;
         y = mY;
         ofRect(x,y,mElementWidth,radioSpacing);
      }
      
      ofNoFill();

      ofSetColor(textColor);
      //ofRect(mX,mY+i*radioSpacing,w,15);
      DrawText(mElements[i].mLabel, x+2, y+12);
   }

   ofPopStyle();
   
   DrawHover();
}

bool RadioButton::MouseMoved(float x, float y)
{
   CheckHover(x, y);
   return false;
}

void RadioButton::OnClicked(int x, int y, bool right)
{
   if (right)
      return;
   
   if (mDirection == kRadioVertical)
      SetIndex(y/radioSpacing);
   else
      SetIndex(int(x/mElementWidth));
}

void RadioButton::SetIndex(int i)
{
   i = ofClamp(i,0,mElements.size()-1);
   int oldVal = *mVar;
   if (mMultiSelect)
      *mVar ^= 1 << mElements[i].mValue;
   else
      *mVar = mElements[i].mValue;
   if (oldVal != *mVar)
   {
      CalcSliderVal();
      mOwner->RadioButtonUpdated(this, oldVal);
      gControlTactileFeedback = 1;
   }
}

void RadioButton::SetFromMidiCC(float slider)
{
   slider = ofClamp(slider,0,1);
   SetIndex(int(slider*mElements.size()));
   mSliderVal = slider;
   mLastSetValue = *mVar;
}

float RadioButton::GetValueForMidiCC(float slider) const
{
   if (mElements.empty())
      return 0;
   
   int index = int(slider*mElements.size());
   index = ofClamp(index,0,mElements.size()-1);
   return mElements[index].mValue;
}

void RadioButton::SetValue(float value)
{
   if (mMultiSelect)
      value = *mVar ^ (1 << (int)value);
   SetValueDirect(value);
}

void RadioButton::SetValueDirect(float value)
{
   int oldVal = *mVar;
   
   *mVar = (int)value;
   if (oldVal != *mVar)
   {
      CalcSliderVal();
      mOwner->RadioButtonUpdated(this, oldVal);
      gControlTactileFeedback = 1;
   }
}

float RadioButton::GetValue() const
{
   return *mVar;
}

float RadioButton::GetMidiValue()
{
   if (mMultiSelect)
      return GetValue();
   
   return mSliderVal;
}

string RadioButton::GetDisplayValue(float val) const
{
   if (mMultiSelect)
      return "multiselect";
   
   int curIndex = -1;
   
   for (int i=0; i<mElements.size(); ++i)
   {
      if (mElements[i].mValue == val)
         curIndex = i;
   }
   
   if (curIndex >= 0 && curIndex < mElements.size())
      return mElements[curIndex].mLabel;
   else
      return "???";
}

void RadioButton::Increment(float amount)
{
   if (mMultiSelect)
      return;
   
   int current = 0;
   for (int i=0; i<mElements.size(); ++i)
   {
      if (mElements[i].mValue == *mVar)
      {
         current = i;
         break;
      }
   }
   
   SetIndex(current + (int)amount);
}

EnumMap RadioButton::GetEnumMap()
{
   EnumMap ret;
   for (int i=0; i<mElements.size(); ++i)
      ret[mElements[i].mLabel] = mElements[i].mValue;
   return ret;
}

void RadioButton::CalcSliderVal()
{
   int current = -1;
   for (int i=0; i<mElements.size(); ++i)
   {
      if (mElements[i].mValue == *mVar)
      {
         current = i;
         break;
      }
   }
   if (current != -1)
      mSliderVal = ofMap(current, 0, mElements.size(), 0, 1);
   else
      mSliderVal = -1;
   mLastSetValue = *mVar;
}

namespace
{
   const int kSaveStateRev = 0;
}

void RadioButton::SaveState(FileStreamOut& out)
{
   out << kSaveStateRev;
   
   out << (float)*mVar;
}

void RadioButton::LoadState(FileStreamIn& in, bool shouldSetValue)
{
   int rev;
   in >> rev;
   LoadStateValidate(rev == kSaveStateRev);
   
   float var;
   in >> var;
   if (shouldSetValue)
      SetValueDirect(var);
}
