//###########################################################################
// This file is part of ProcessLib, a submodule of LImA project the
// Library for Image Acquisition
//
// Copyright (C) : 2009-2011
// European Synchrotron Radiation Facility
// BP 220, Grenoble 38043
// FRANCE
//
// This is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 3 of the License, or
// (at your option) any later version.
//
// This software is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, see <http://www.gnu.org/licenses/>.
//###########################################################################
#include "Binning.h"
#include "Stat.h"
#include <iostream>
#include <sstream>
using namespace Tasks;

//static function
template<class INPUT> static INPUT max_value(const INPUT &)
{
  return INPUT((1ULL << (8 * sizeof(INPUT))) - 1);
}

/** @brief generique binning but not optimized at all
 */
template<class INPUT>
inline void _default_binning(Data &aSrcData,Data &aDstData,
			     int xFactor,int yFactor)
{
  INPUT *aSrcPt = (INPUT*)aSrcData.data();
  INPUT *aDstPt = (INPUT*)aDstData.data();
  INPUT MAX_VALUE = max_value(*aSrcPt);
  int *lineOffset = new int[yFactor];
  int aTmpOffSet = 0;
  for(int linId = 0;linId < yFactor;++linId,aTmpOffSet += aSrcData.dimensions[0])
    lineOffset[linId] = aTmpOffSet;

  INPUT *aLineSrcPt = aSrcPt;
  INPUT *aLineDstPt = aDstPt;
  for(int lineId = 0;lineId <= (aSrcData.dimensions[1] - yFactor);
      lineId += yFactor,aLineSrcPt += aSrcData.dimensions[0],aLineDstPt += aDstData.dimensions[0])
    {
      int aDstColumnId = 0;
      for(int columnId = 0;columnId <= (aSrcData.dimensions[0] - xFactor);
	  columnId += xFactor,++aDstColumnId)
	{
	  unsigned long long result = 0;
	  for(int aPixelLine = 0;aPixelLine < yFactor;++aPixelLine)
	    for(int aPixelColumn = columnId;aPixelColumn < columnId + xFactor;++aPixelColumn)
	      result += *(aLineSrcPt + lineOffset[aPixelLine] + aPixelColumn);

	  if(result > MAX_VALUE)
	    aLineDstPt[aDstColumnId] = MAX_VALUE;
	  else
	    aLineDstPt[aDstColumnId] = INPUT(result);
	}
    }
  delete lineOffset;
}

/** @brief binning 2 x 2
 */
template<class INPUT>
static void _binning2x2(Data &aSrcData,Data &aDstData,int Factor)
{

  INPUT *aSrcFirstLinePt = (INPUT*)aSrcData.data();
  INPUT *aSrcSecondLinePt = aSrcFirstLinePt + aSrcData.dimensions[0];
  INPUT *aDstPt = (INPUT*)aDstData.data();
  INPUT MAX_VALUE = max_value(*aDstPt);
  for(int lineId = 0;lineId < aSrcData.dimensions[1];lineId += 2)
    {
      for(int columnId = 0;columnId < aSrcData.dimensions[0];columnId += 2,
	    ++aDstPt,aSrcFirstLinePt += 2,aSrcSecondLinePt += 2)
	{
	  unsigned long long result = *aSrcFirstLinePt + *(aSrcFirstLinePt + 1) +
	    *aSrcSecondLinePt + *(aSrcSecondLinePt + 1);
	  if(result > MAX_VALUE)
	    *aDstPt = MAX_VALUE;
	  else
	    *aDstPt = INPUT(result);
	}
      aSrcFirstLinePt = aSrcSecondLinePt;
      aSrcSecondLinePt = aSrcFirstLinePt + aSrcData.dimensions[0];
    }
  aDstData.width >>= 1;
  aDstData.height >>= 1;
  if(Factor > 2)
    _binning2x2<INPUT>(aDstData,aDstData,Factor >> 1);
}

Binning::Binning() : mXFactor(-1),mYFactor(-1) {};
Binning::Binning(const Binning &anOther) :
  LinkTask(anOther),
  mXFactor(anOther.mXFactor),
  mYFactor(anOther.mYFactor) {}

Data Binning::process(Data &aData)
{
  Data aNewData = aData;
  if(aData.dimensions.size() != 2)
    std::cerr << "Binning : Only manage 2D data " << std::endl;
  else if(!aData.empty())
    {
      std::stringstream info;
      info << "Binning " << mXFactor << " by " << mYFactor;
      Stat aStat(aNewData,info.str());
      if(mYFactor > 0 && mXFactor > 0)
	{
	  if((mXFactor == 2 && mYFactor == 2) ||
	     (mXFactor == 4 && mYFactor == 4) ||
	     (mXFactor == 8 && mYFactor == 8) ||
	     (mXFactor == 16 && mYFactor == 16) ||
	     (mXFactor == 32 && mYFactor == 32)) // Factor 2 (Most used)
	    {
	      if(!_processingInPlaceFlag)
		{
		  Buffer *aNewBuffer = new Buffer(aData.size() >> 2);
		  aNewData.setBuffer(aNewBuffer);
		  aNewBuffer->unref();
		}

	      switch(aData.type)
		{
		case Data::UINT8:
		  _binning2x2<unsigned char>(aData,aNewData,mXFactor);break;
		case Data::UINT16:
		  _binning2x2<unsigned short>(aData,aNewData,mXFactor);break;
		case Data::UINT32:
		  _binning2x2<unsigned int>(aData,aNewData,mXFactor);break;
		default:
		  std::cerr << "Binning : Data type not managed" << std::endl;
		  break;
		}	  
	    }
	  else			// DEFAULT case is not optimized
	    {
	      if(!_processingInPlaceFlag)
		{
		  
		  Buffer *aNewBuffer = new Buffer(aData.size() >> 2);
		  aNewData.setBuffer(aNewBuffer);
		  aNewBuffer->unref();
		}

	      switch(aData.type)
		{
		case Data::UINT8:
		  _default_binning<unsigned char>(aData,aNewData,mXFactor,mYFactor);break;
		case Data::UINT16:
		  _default_binning<unsigned short>(aData,aNewData,mXFactor,mYFactor);break;
		case Data::UINT32:
		  _default_binning<unsigned int>(aData,aNewData,mXFactor,mYFactor);break;
		default:
		  std::cerr << "Binning : Data type not managed" << std::endl;
		  break;
		}
	       aNewData.width /= mXFactor;
	       aNewData.height /= mYFactor;
	    }
	}
      else
	std::cerr << "Binning : Factor as not been set" << std::endl;
    }
  else
    std::cerr << "Binning : Data is empty!" << std::endl;
  return aNewData;
}

