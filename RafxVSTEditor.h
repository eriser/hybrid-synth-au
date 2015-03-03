
#ifndef __CRafxVSTEditor__
#define __CRafxVSTEditor__

#include "WPXMLParser.h"
#include "plugin.h"
#include "synthfunctions.h"
#include "VuMeterWP.h"
#include "XYPadWP.h"
#include "rafx2vstguimessages.h"
#include "vstgui4/vstgui/vstgui.h"

#include <sstream>
#import <CoreAudio/CoreAudio.h>
#import <AudioUnit/AudioUnit.h>
#import <AudioToolbox/AudioToolbox.h>

using namespace std;
using namespace pugi;

namespace VSTGUI {

const UINT ASSIGNBUTTON_1 = 32768;
const UINT ASSIGNBUTTON_2 = 32769;
const UINT ASSIGNBUTTON_3 = 32770;
const UINT ALPHA_WHEEL    = 32771;
const UINT LCD_KNOB		  = 32772;
const UINT JOYSTICK_X	  = 32773;
const UINT JOYSTICK_Y	  = 32774;
const UINT JOYSTICK 	  = 32775;
const UINT TRACKPAD 	  = 32776;

// --- custom RAFX Attributes
const UINT RAFX_XML_NODE = 32768;
const UINT RAFX_TEMPLATE_TYPE = 32769;
const UINT RAFX_CONTROL_TYPE = 32770;

typedef struct
{
    void* pWindow;
    float width;
    float height;
    AudioUnit au;
} VIEW_STRUCT;

class CRafxVSTEditor : public VSTGUIEditorInterface, public CControlListener, public CBaseObject
{
public:
	CRafxVSTEditor();
    ~CRafxVSTEditor();
    CVSTGUITimer* timer;
   	CMessageResult notify(CBaseObject* sender, const char* message);
    bool m_bInitialized;

    // --- added
    void getSize(float& width, float& height);

	CWPXMLParser m_XMLParser;
	void* m_pRafxFrame;

	bool bitmapExistsInResources(const char* bitmapname);

	bool open(void* pHwnd, char* pXMLFile, AudioUnit inAU);
    void close();
	bool bClosing;

	// --- lin/circ
	UINT m_uKnobAction;
	virtual int32_t getKnobMode() const;

	// from CControlListener
	void valueChanged(CControl* pControl);
    float getCookedValue(int nIndex, float fNormalizedValue);
	virtual void idle();

	// -- WP RAFX
	void initControls(bool bSetListener = true);
	CBitmap* loadBitmap(const CResourceDescription& desc);
	CNinePartTiledBitmap* loadTiledBitmap(const CResourceDescription& desc, CCoord left, CCoord top, CCoord right, CCoord bottom);

	bool createSubView(CViewContainer* pParentView, pugi::xml_node viewNode, bool bFrameSubView); // framesubview = not part of anothe vc part of frame vc
	bool deleteSubView(CViewContainer* pParentView, pugi::xml_node viewNode, pugi::xml_node parentNode); // framesubview = not part of anothe vc part of frame vc

	CTextLabel* createTextLabel(pugi::xml_node node, bool bAddingNew = true, bool bStandAlone = false);
	void parseTextLabel(CTextLabel* pLabel, pugi::xml_node node);
	CTextEdit* createTextEdit(pugi::xml_node node, bool bAddingNew = true, bool bStandAlone = false);
	CFontDesc* createFontDesc(pugi::xml_node node);
	CFontDesc* createFontDescFromFontNode(pugi::xml_node node);
	CAnimKnob* createAnimKnob(pugi::xml_node node, CBitmap* bitmap, bool bAddingNew = true, bool bStandAlone = false);
	CSlider* createSlider(pugi::xml_node node, CBitmap* handleBitmap, CBitmap* grooveBitmap, bool bAddingNew = true, bool bStandAlone = false);
	CVerticalSwitch* createVerticalSwitch(pugi::xml_node node, CBitmap* backgroundBitmap, bool bAddingNew = true, bool bStandAlone = false);
	COnOffButton* createOnOffButton(pugi::xml_node node, CBitmap* bitmap, bool bAddingNew = true, bool bStandAlone = false);
	CVuMeterWP* createMeter(pugi::xml_node node, CBitmap* onBitmap, CBitmap* offBitmap, bool bAddingNew = true, bool bStandAlone = false);
	CKickButton* createKickButton(pugi::xml_node node, CBitmap* backgroundBitmap, bool bAddingNew = true, bool bStandAlone = false);
	COptionMenu* createOptionMenu(pugi::xml_node node, CBitmap* backgroundBitmap, bool bAddingNew = true, bool bStandAlone = false);
	CXYPadWP* createXYPad(pugi::xml_node node, CBitmap* backgroundBitmap, bool bAddingNew = true, bool bStandAlone = false);
	CViewContainer* createViewContainer(pugi::xml_node node, CBitmap* backgroundBitmap, bool bAddingNew = true, bool bStandAlone = false);

	CTextLabel* m_pLCDControlNameLabel;
	CTextLabel* m_pLCDControlIndexCountLabel;

	void setPlugIn(CPlugIn* pPlugIn){m_pPlugIn = pPlugIn;}
	void setControlMap(UINT* pControlMap){m_pControlMap = pControlMap;}
	void setLCDControlMap(UINT* pLCDControlMap){m_pLCDControlMap = pLCDControlMap;}
	void setLCDControlCount(int nCount){m_nLCDControlCount = nCount;}

	// --------------------------------------------------------------------------------------------------
	const char* getBitmapName(int nIndex);
	inline bool isBitmapTiled(const char* bitmapName, int& left, int& top, int& right, int& bottom)
	{
		left = 0;
		top = 0;
		right = 0;
		bottom = 0;

		if(m_XMLParser.isBitmapTiled(bitmapName))
		{
			const char_t* offsets = m_XMLParser.getBitmapAttribute(bitmapName, "nineparttiled-offsets");
			CCoord Dleft, Dtop, Dright, Dbottom = 0;
			getTiledOffsets(offsets, Dleft, Dtop, Dright, Dbottom);
			left = (int)Dleft;
			right = (int)Dright;
			top = (int)Dtop;
			bottom = (int)Dbottom;
			return true;
		}
		return false;
	}
	const char* getCurrentBackBitmapName();
	const char* getCurrentBackColorName();
	const char* getRAFXBitmapType(const char* bitmapName);
	bool setRAFXBitmapType(const char* bitmapName, const char* type);
	bool getBitmapsWithRAFXType(std::vector<std::string>* pStringList, const char* type, bool bAppend, bool bIncludeNoTypes);
	bool getColors(std::vector<std::string>* pNameList, std::vector<std::string>* pRGBAList);
	bool getFonts(std::vector<std::string>* pFontNameList);
	bool getTags(std::vector<std::string>* pControlTagList, UINT uControlType, bool bClearList = true);
	bool getJoystickTags(std::vector<std::string>* pControlTagList);

	// for radio buttons...
	const char* getEnumString(const char* sTag);
    inline bool setCurrentBackBitmap(const char* bitmap, CFrame* pFrame = NULL)
	{
		if(pFrame == NULL)
			pFrame = frame;

		if(strcmp(bitmap, "none") == 0)
		{
			pFrame->setBackground(NULL);
			pFrame->invalid();
			pFrame->idle();

			int32_t outSize;
			pugi::xml_node node;
			if(pFrame->getAttribute(RAFX_XML_NODE, sizeof(pugi::xml_node*), &node, outSize))
				m_XMLParser.setAttribute(node, "bitmap", "");

			return true;
		}

		CNinePartTiledBitmap* backgroundTiledBMP;
		CBitmap* backgroundBMP;

		// --- lookup the bitmap from the file
		if(m_XMLParser.hasBitmap(bitmap))
		{
			// --- get the bitmap file name (png)
			const char_t* bitmapFilename = m_XMLParser.getBitmapAttribute(bitmap, "path");

			if(m_XMLParser.isBitmapTiled(bitmap))
			{
				const char_t* offsets = m_XMLParser.getBitmapAttribute(bitmap, "nineparttiled-offsets");
				CCoord left, top, right, bottom = 0;
				getTiledOffsets(offsets, left, top, right, bottom);

				backgroundTiledBMP = loadTiledBitmap(bitmapFilename, left, top, right, bottom);
				pFrame->setBackground(backgroundTiledBMP);
				if(backgroundTiledBMP)
					backgroundTiledBMP->forget();
			}
			else
			{
				backgroundBMP = loadBitmap(bitmapFilename);
				pFrame->setBackground(backgroundBMP);
				if(backgroundBMP)
					backgroundBMP->forget();
			}

			int32_t outSize;
			pugi::xml_node node;
			if(pFrame->getAttribute(RAFX_XML_NODE, sizeof(pugi::xml_node*), &node, outSize))
				m_XMLParser.setAttribute(node, "bitmap", bitmap);

			return true;
		}

		return false;
	}

	inline bool setCurrentBackColor(const char* color, COLORREF cr, CFrame* pFrame = NULL)
	{
		if(pFrame == NULL)
			pFrame = frame;

		string str(color);

		CColor backCColor;
		CColor builtInColor;

		// --- add if not existing
		checkAddCColor(color, cr);

		if(isBuiltInColor(str, builtInColor))
			backCColor = builtInColor;
		else if(!m_XMLParser.hasColor(color))
			return false;
		else
			backCColor = getCColor(color);

		// --- set on frame
        //	CColor backCColor = getCColor(color);
		pFrame->setBackgroundColor(backCColor);

		// --- save
		int32_t outSize;
		pugi::xml_node node;
		if(pFrame->getAttribute(RAFX_XML_NODE, sizeof(pugi::xml_node*), &node, outSize))
			m_XMLParser.setAttribute(node, "background-color", color);

		return true;
	}

	// --- helpers for the massive VC get/set shit
	inline void getStringAttribute(pugi::xml_node node, const char* attributeName, std::string* destination)
	{
		const char* att =  m_XMLParser.getAttribute(node, attributeName);
		if(att)
		{
			string s(att);
			*destination = s;
		}
	}
	inline void setStringAttribute(pugi::xml_node node, const char* attributeName, std::string value)
	{
		m_XMLParser.setAttribute(node, attributeName, value.c_str());
	}
	inline void setAttribute(pugi::xml_node node, const char* attributeName, const char* attributeValue)
	{
		m_XMLParser.setAttribute(node, attributeName, attributeValue);
	}
	inline void getIntAttribute(pugi::xml_node node, const char* attributeName, int* destination)
	{
		const char* att =  m_XMLParser.getAttribute(node, attributeName);
		if(att)
		{
			string s(att);
			*destination = atoi(s.c_str());
		}
	}
	inline void setIntAttribute(pugi::xml_node node, const char* attributeName, int value)
	{
		char* text =  new char[33];
		itoa(value, text, 10);
		const char* cctext(text);
		m_XMLParser.setAttribute(node, attributeName, cctext);
		delete []  text;
	}
	inline void getBoolAttribute(pugi::xml_node node, const char* attributeName, bool* destination, bool bReverseLogic = false)
	{
		const char* att =  m_XMLParser.getAttribute(node, attributeName);
		if(att)
		{
			if(bReverseLogic)
			{
				*destination = true;
				if(strcmp(att, "true") == 0)
					*destination = false;
			}
			else
			{
				*destination = false;
				if(strcmp(att, "true") == 0)
					*destination = true;
			}
		}
	}
	inline void setBoolAttribute(pugi::xml_node node, const char* attributeName, bool value)
	{
		const char* pAtt = value ? "true" : "false";
		m_XMLParser.setAttribute(node, attributeName, pAtt);
	}

	// --- CREATION FUNCTIONS
	inline CFontDesc* createFontDesc(const char* font, int nWeight,
									 bool bBold = false, bool bItalic = false,
									 bool bStrike = false, bool bUnder = false)
	{
		const char_t* fontname = m_XMLParser.getFontAttribute(font, "font-name");
		string str(fontname);
		CFontDesc* fontDesc = NULL;
		if(strlen(fontname) <= 0)
		{
			const CFontRef builtInFont = getBuiltInFont(font);
			if(builtInFont)
			{
				fontDesc = builtInFont;
			}
		}
		else
			fontDesc = new CFontDesc(fontname);

		//
		const char_t* fontSize = intToString(nWeight);
		if(strlen(fontSize) > 0)
		{
			string ccoord(fontSize);
			const CCoord fontsize = ::atof(ccoord.c_str());
			fontDesc->setSize(fontsize);
		}

		// --- bold
		if(bBold)
			fontDesc->setStyle(kBoldFace);

		// --- ital
		if(bItalic)
			fontDesc->setStyle(fontDesc->getStyle() | kItalicFace);

		if(bStrike)
			fontDesc->setStyle(fontDesc->getStyle() | kStrikethroughFace);

		if(bUnder)
			fontDesc->setStyle(fontDesc->getStyle() | kUnderlineFace);

		return fontDesc;
	}

	// --- true if not VC
	inline bool isNotVC(string sClass)
	{
		if(sClass != "CViewContainer" &&
           sClass != "CSplitView" &&
           sClass != "CLayeredViewContainer" &&
           sClass != "CRowColumnView" &&
           sClass != "CRowColumnView" &&
           sClass != "CScrollView" &&
           sClass != "CShadowViewContainer" &&
           sClass != "UIViewSwitchContainer")
			return true;

		return false;
	}

	// --- currently only supporting this, may add more later
	inline bool isSupportedVC(string sClass)
	{
		if(sClass == "CViewContainer")
			return true;

		return false;
	}

	inline const CRect getRectFromNode(pugi::xml_node viewNode)
	{
		// --- make rect
		const char_t* origin = m_XMLParser.getAttribute(viewNode, "origin");
		string sOrigin = string(origin);

		int x, y = 0;
		if(!getXYFromString(sOrigin, x, y))
		{
			const CRect rect(0,0,0,0);
			return rect;
		}

		const char_t* size = m_XMLParser.getAttribute(viewNode, "size");
		string sSize = string(size);

		int w, h = 0;
		if(!getXYFromString(sSize, w, h))
		{
			const CRect rect(0,0,0,0);
			return rect;
		}
		const CRect rect(x, y, x+w, y+h);
		return rect;
	}
	inline bool getXYFromString(string str, int& x, int& y)
	{
		if(str.length() <= 0)
			return false;

		int nComma = str.find_first_of(',');
		if(nComma <= 0)
			return false;

		string sX = str.substr(0, nComma);
		string sY = str.substr(nComma+1);

		x = atoi(sX.c_str());
		y = atoi(sY.c_str());
		return true;
	}
	inline string getStringFromCPoint(CPoint point)
	{
		string sX;
		string sY;
		string str;

		ostringstream strs;
		strs << point.x;
		sX.assign(strs.str());

		ostringstream strs2;
		strs2 << point.y;
		sY.assign(strs2.str());

		str.assign(sX);
		str.append(", ");
		str.append(sY);
		return str;
	}
	inline const CPoint getCPointFromString(string offset)
	{
		int x, y = 0;
		getXYFromString(offset, x, y);
		return CPoint(x,y);
	}
	inline bool getTiledOffsets(string str, CCoord& left, CCoord& top, CCoord& right, CCoord& bottom)
	{
		left = 0; top = 0; right = 0; bottom = 0;
		if(str.length() <= 0)
			return false;

		int nComma = str.find_first_of(',');
		if(nComma <= 0)
			return false;

		string sLeft = str.substr(0, nComma);
		string sRem1 = str.substr(nComma+1);

		nComma = sRem1.find_first_of(',');
		if(nComma <= 0)
			return false;

		string sTop = sRem1.substr(0, nComma);
		string sRem2 = sRem1.substr(nComma+1);

		nComma = sRem2.find_first_of(',');
		if(nComma <= 0)
			return false;

		string sRight = sRem2.substr(0, nComma);
		string sBottom = sRem2.substr(nComma+1);

		left = atof(sLeft.c_str());
		top = atof(sTop.c_str());
		right = atof(sRight.c_str());
		bottom = atof(sBottom.c_str());

		return true;
	}
	inline bool getRGBAFromString(string str, int& r, int& g, int& b, int& a)
	{
		if(str.length() <= 0)
			return false;

		string strAlpha = str.substr(7,2);
		stringstream ss1(strAlpha);
		ss1 >> hex >> a;

		string strR = str.substr(1,2);
		string strG = str.substr(3,2);
		string strB = str.substr(5,2);

		stringstream ss2(strR);
		ss2 >> hex >> r;
		stringstream ss3(strG);
		ss3 >> hex >> g;
		stringstream ss4(strB);
		ss4 >> hex >> b;
		return true;
	}
	inline bool isBuiltInColor(string str, CColor& builtInColor)
	{
		char c = str.at(0);
		if(c == '~')
		{
			if(strcmp(str.c_str(), "~ TransparentCColor") == 0)
			{
				builtInColor = kTransparentCColor; return true;
			}
			if(strcmp(str.c_str(), "~ BlackCColor") == 0)
			{
				builtInColor = kBlackCColor; return true;
			}
			if(strcmp(str.c_str(), "~ WhiteCColor") == 0)
			{
				builtInColor = kWhiteCColor; return true;
			}
			if(strcmp(str.c_str(), "~ GreyCColor") == 0)
			{
				builtInColor = kGreyCColor; return true;
			}
			if(strcmp(str.c_str(), "~ RedCColor") == 0)
			{
				builtInColor = kRedCColor; return true;
			}
			if(strcmp(str.c_str(), "~ GreenCColor") == 0)
			{
				builtInColor = kGreenCColor; return true;
			}
			if(strcmp(str.c_str(), "~ BlueCColor") == 0)
			{
				builtInColor = kBlueCColor; return true;
			}
			if(strcmp(str.c_str(), "~ YellowCColor") == 0)
			{
				builtInColor = kYellowCColor; return true;
			}
			if(strcmp(str.c_str(), "~ CyanCColor") == 0)
			{
				builtInColor = kCyanCColor; return true;
			}
			if(strcmp(str.c_str(), "~ MagentaCColor") == 0)
			{
				builtInColor = kMagentaCColor; return true;
			}
		}
		return false;
	}
	inline const CFontRef getBuiltInFont(string str)
	{
		char c = str.at(0);
		if(c == '~')
		{
			if(strcmp(str.c_str(), "~ SystemFont") == 0)
				return kSystemFont;
			if(strcmp(str.c_str(), "~ NormalFontVeryBig") == 0)
				return kNormalFontVeryBig;
			if(strcmp(str.c_str(), "~ NormalFontBig") == 0)
				return kNormalFontBig;
			if(strcmp(str.c_str(), "~ NormalFont") == 0)
				return kNormalFont;
			if(strcmp(str.c_str(), "~ NormalFontSmall") == 0)
				return kNormalFontSmall;
			if(strcmp(str.c_str(), "~ NormalFontSmaller") == 0)
				return kNormalFontSmaller;
			if(strcmp(str.c_str(), "~ NormalFontVerySmall") == 0)
				return kNormalFontVerySmall;
			if(strcmp(str.c_str(), "~ SymbolFont") == 0)
				return kSymbolFont;
		}
		return NULL;
	}
	inline CColor getCColor(const char_t* colorname)
	{
		if(strlen(colorname) <= 0)
			return CColor(0, 0, 0, 1);

		string sColorName(colorname);
		CColor theCColor;
		if(isBuiltInColor(sColorName, theCColor))
			return theCColor;
		else
		{
			// do we have it?
			if(m_XMLParser.hasColor(colorname))
			{
				const char_t* rgba = m_XMLParser.getColorAttribute(colorname, "rgba");
				string sRGBA(rgba);
				int r, g, b, a = 255;
				getRGBAFromString(sRGBA, r, g, b, a);
				return CColor(r, g, b, a);
			}
		}
		// --- may be empty/black
		return theCColor;
	}
	inline string COLORREFtoRGBAstring(DWORD cr)
	{
		char cResultR[32];
		char cResultG[32];
		char cResultB[32];
		BYTE r = GetRValue(cr);	/* get R, G, and B out of DWORD */
		BYTE g = GetGValue(cr);
		BYTE b = GetBValue(cr);
		sprintf(cResultR, "%X", r);
		sprintf(cResultG, "%X", g);
		sprintf(cResultB, "%X", b);
		string sR(cResultR);
		string sG(cResultG);
		string sB(cResultB);
		if(sR.size() == 1) sR = "0" + sR;
		if(sG.size() == 1) sG = "0" + sG;
		if(sB.size() == 1) sB = "0" + sB;
		string sResult = "#";
		sResult.append(sR);
		sResult.append(sG);
		sResult.append(sB);
		sResult.append("ff");

		return sResult;
	}
	inline bool checkAddCColor(const char_t* colorname, COLORREF cr)
	{
		if(strlen(colorname) <= 0)
			return false;

		if(m_XMLParser.hasColor(colorname))
			return true;

		CColor color;
		if(isBuiltInColor(colorname, color))
			return true;

		// --- make the string
		string sRGBA = COLORREFtoRGBAstring(cr);

		// --- add color
		return m_XMLParser.addColor(colorname, sRGBA.c_str());
	}
	inline CBitmap* getLoadBitmap(pugi::xml_node viewNode, const char_t* bmName = "bitmap")
	{
		const char_t* bitmapname = m_XMLParser.getAttribute(viewNode, bmName);
		const char_t* bitmapFilename = m_XMLParser.getBitmapAttribute(bitmapname, "path");

        if(strlen(bitmapname) <= 0)return NULL;
        if(strlen(bitmapFilename) <= 0)return NULL;

		CNinePartTiledBitmap* backgroundTiledBMP = NULL;
		CBitmap* backgroundBMP = NULL;

		if(m_XMLParser.isBitmapTiled(bitmapname))
		{
			const char_t* offsets = m_XMLParser.getBitmapAttribute(bitmapname, "nineparttiled-offsets");
			CCoord left, top, right, bottom = 0;
			getTiledOffsets(offsets, left, top, right, bottom);

			backgroundTiledBMP = loadTiledBitmap(bitmapFilename, left, top, right, bottom);
			return backgroundTiledBMP;
		}
		else
		{
			backgroundBMP = loadBitmap(bitmapFilename);
			return backgroundBMP;
		}

		return NULL;
	}
	inline int getEnumStringIndex(char* enumString, const char* testString)
	{
		string sEnumStr(enumString);
		string sTestStr(testString);
		int index = 0;
		bool bWorking = true;
		while(bWorking)
		{
			int nComma = sEnumStr.find_first_of(',');
			if(nComma <= 0)
			{
				if(sEnumStr == sTestStr)
					return index;

				bWorking = false;
			}
			else
			{
				string sL = sEnumStr.substr(0, nComma);
				sEnumStr = sEnumStr.substr(nComma+1);

				if(sL == sTestStr)
					return index;

				index++;
			}
		}

		return -1;
	}
    
   	inline int getNumEnums(char* string)
	{
		if(strlen(string) <= 0)
			return 0;
		std::string sTemp(string);
		std::string s(string);
		s.erase(std::remove(s.begin(), s.end(), ','), s.end());
		int nLen = sTemp.size() - s.size();
		return nLen+1;
	}
    
	inline char* getEnumString(char* string, int index)
	{
		int nLen = strlen(string);
		char* copyString = new char[nLen+1];

		strncpy(copyString, string, nLen);
        copyString[nLen] = '\0';

		for(int i=0; i<index+1; i++)
		{
			char * comma = ",";

			int j = strcspn (copyString,comma);

			if(i==index)
			{
				char* pType = new char[j+1];
				strncpy (pType, copyString, j);
				pType[j] = '\0';
				delete [] copyString;

				// special support for 2-state switches
				// (new in RAFX 5.4.14)
				if(strcmp(pType, "SWITCH_OFF") == 0)
				{
					delete [] pType;
					return "OFF";
				}
				else if(strcmp(pType, "SWITCH_ON") == 0)
				{
					delete [] pType;
					return "ON";
				}

				return pType;
			}
			else // remove it
			{
				char* pch = strchr(copyString,',');

				if(!pch)
				{
					delete [] copyString;
					return NULL;
				}

				int nLen = strlen(copyString);
				memcpy (copyString,copyString+j+1,nLen-j);
			}
		}

		delete [] copyString;
		return NULL;
	}

    inline void broadcastControlChange(int nTag, float fPluginValue, float fControlValue, CControl* pControl, CUICtrl* pCtrl)
	{
		if(isOptionMenuControl(pControl))
			fControlValue = fPluginValue;
		else if(isRadioButtonControl(pControl))
		{
			fControlValue = pControl->getValueNormalized();
			fPluginValue = fControlValue;
		}

		for(int i=0; i<m_nAnimKnobCount; i++)
		{
			if(m_ppKnobControls[i] == pControl)
				continue;

			if(m_ppKnobControls[i]->getTag() == nTag)
			{
				m_ppKnobControls[i]->setValue(fControlValue);
				m_ppKnobControls[i]->invalid();
			}
			else if(m_ppKnobControls[i]->getTag() == LCD_KNOB)
			{
				if(m_pLCDControlMap[m_nAlphaWheelIndex] == nTag)
				{
					m_ppKnobControls[i]->setValue(fControlValue);
					m_ppKnobControls[i]->invalid();

					CUICtrl* pCtrl = m_pPlugIn->m_UIControlList.getAt(m_pLCDControlMap[m_nAlphaWheelIndex]);

					// --- update the edit control
					updateTextEdits(LCD_KNOB, m_ppKnobControls[i], pCtrl);
				}
			}
		}
		for(int i=0; i<m_nSliderCount; i++)
		{
			if(m_ppSliderControls[i] == pControl)
				continue;

			if(m_ppSliderControls[i]->getTag() == nTag)
			{
				m_ppSliderControls[i]->setValue(fControlValue);
				m_ppSliderControls[i]->invalid();
			}
		}
		for(int i=0; i<m_nKickButtonCount; i++)
		{
			if(m_ppKickButtonControls[i] == pControl)
				continue;

			if(m_ppKickButtonControls[i]->getTag() == nTag)
			{
				m_ppKickButtonControls[i]->setValue(fControlValue);
				m_ppKickButtonControls[i]->invalid();
			}
		}
		for(int i=0; i<m_nOnOffButtonCount; i++)
		{
			if(m_ppOnOffButtonControls[i] == pControl)
				continue;

			if(m_ppOnOffButtonControls[i]->getTag() == nTag)
			{
				m_ppOnOffButtonControls[i]->setValue(fControlValue);
				m_ppOnOffButtonControls[i]->invalid();
			}
		}
		for(int i=0; i<m_nRadioButtonCount; i++)
		{
			if(m_ppRadioButtonControls[i] == pControl)
				continue;

			if(m_ppRadioButtonControls[i]->getTag() == nTag)
			{
				float fV = fPluginValue*(float)(m_ppRadioButtonControls[i]->getMax());
				m_ppRadioButtonControls[i]->setValue((int)fV);
				m_ppRadioButtonControls[i]->invalid();
			}
		}
		for(int i=0; i<m_nVuMeterCount; i++)
		{
			if(m_ppVuMeterControls[i] == pControl)
				continue;

			if(m_ppVuMeterControls[i]->getTag() == nTag)
			{
				m_ppVuMeterControls[i]->setValue(fControlValue);
				m_ppVuMeterControls[i]->invalid();
			}
		}
		// --- text edits
		updateTextEdits(nTag, pControl, pCtrl);

		// --- option menus
		for(int i=0; i<m_nOptionMenuCount; i++)
		{
			if(m_ppOptionMenuControls[i] == pControl)
				continue;

			if(m_ppOptionMenuControls[i]->getTag() == nTag)
			{
				float fV = fPluginValue*(float)(m_ppOptionMenuControls[i]->getNbEntries() - 1);
				m_ppOptionMenuControls[i]->setCurrent((int)fV);
				m_ppOptionMenuControls[i]->invalid();
			}
		}
		for(int i=0; i<m_nXYPadCount; i++)
		{
			if(m_ppXYPads[i] == pControl)
				continue;

			if(m_ppXYPads[i]->getTagX() == nTag || m_ppXYPads[i]->getTagY() == nTag)
			{
				float x, y;
				m_ppXYPads[i]->calculateXY(m_ppXYPads[i]->getValue(), x, y);
				y = -1.0*y + 1.0;
				if(m_ppXYPads[i]->getTagX() == nTag)
					x = fControlValue;
				else
					y = fControlValue;
				float val = m_ppXYPads[i]->calculateValue(x, y);
				m_ppXYPads[i]->setValue(val);
				m_ppXYPads[i]->invalid();
			}
		}
	}

	inline CAnimKnob* getAlphaWheelKnob()
	{
		for(int i=0; i<m_nAnimKnobCount; i++)
		{
			if(m_ppKnobControls[i]->getTag() == ALPHA_WHEEL)
				return m_ppKnobControls[i];
		}
		return NULL;
	}

	inline CAnimKnob* getLCDValueKnob()
	{
		for(int i=0; i<m_nAnimKnobCount; i++)
		{
			if(m_ppKnobControls[i]->getTag() == LCD_KNOB)
				return m_ppKnobControls[i];
		}
		return NULL;
	}

	inline CTextEdit* getLCDTextEdit()
	{
		for(int i=0; i<m_nTextEditCount; i++)
		{
			if(m_ppTextEditControls[i]->getTag() == LCD_KNOB)
				return m_ppTextEditControls[i];
		}
		return NULL;
	}

	inline CXYPadWP* findJoystickXYPad(CControl* pControl)
	{
		for(int i=0; i<m_nXYPadCount; i++)
		{
			if(m_ppXYPads[i] == (CXYPadWP*)pControl)
				return m_ppXYPads[i];
		}
		return NULL;
	}

	// --- called when value changes in text edit; it needs to store the new stringindex value and update itself
	inline float updateEditControl(CControl* pControl, CUICtrl* pCtrl)
	{
		const char* p = ((CTextEdit*)pControl)->getText();

		float fValue = 0.0;
		switch(pCtrl->uUserDataType)
		{
			case floatData:
			{
				float f = atof(p);
				if(f > pCtrl->fUserDisplayDataHiLimit) f = pCtrl->fUserDisplayDataHiLimit;
				if(f < pCtrl->fUserDisplayDataLoLimit) f = pCtrl->fUserDisplayDataLoLimit;
				p = floatToString(f,2);
				fValue = calcSliderVariable(pCtrl->fUserDisplayDataLoLimit, pCtrl->fUserDisplayDataHiLimit, f);
				pControl->setValue(fValue);
				((CTextEdit*)pControl)->setText(p);
				break;
			}
			case doubleData:
			{
				float f = atof(p);
				if(f > pCtrl->fUserDisplayDataHiLimit) f = pCtrl->fUserDisplayDataHiLimit;
				if(f < pCtrl->fUserDisplayDataLoLimit) f = pCtrl->fUserDisplayDataLoLimit;
				p = floatToString(f,2);
				fValue = calcSliderVariable(pCtrl->fUserDisplayDataLoLimit, pCtrl->fUserDisplayDataHiLimit, f);
				pControl->setValue(fValue);
				((CTextEdit*)pControl)->setText(p);
				break;
			}
			case intData:
			{
				int f = atoi(p);
				if(f > pCtrl->fUserDisplayDataHiLimit) f = pCtrl->fUserDisplayDataHiLimit;
				if(f < pCtrl->fUserDisplayDataLoLimit) f = pCtrl->fUserDisplayDataLoLimit;
				p = intToString(f);
				fValue = calcSliderVariable(pCtrl->fUserDisplayDataLoLimit, pCtrl->fUserDisplayDataHiLimit, f);
				pControl->setValue(fValue);
				((CTextEdit*)pControl)->setText(p);
				break;
			}
			case UINTData:
			{
				string str(p);
				string list(pCtrl->cEnumeratedList);
				if(list.find(str) == -1)
				{
					fValue = calcSliderVariable(pCtrl->fUserDisplayDataLoLimit, pCtrl->fUserDisplayDataHiLimit, *(pCtrl->m_pUserCookedUINTData));
					pControl->setValue(fValue);
					//((CTextEdit*)pControl)->setText(str.c_str());

					char* pEnum;
					pEnum = getEnumString(pCtrl->cEnumeratedList, (int)(*(pCtrl->m_pUserCookedUINTData)));
					if(pEnum)
						((CTextEdit*)pControl)->setText(pEnum);
				}
				else
				{
					int t = getEnumStringIndex(pCtrl->cEnumeratedList, p);
					if(t < 0)
					{
						// this should never happen...
						char* pEnum;
						pEnum = getEnumString(pCtrl->cEnumeratedList, 0);
						if(pEnum)
							((CTextEdit*)pControl)->setText(pEnum);
						fValue = 0.0;
					}
					else
					{
						fValue = calcSliderVariable(pCtrl->fUserDisplayDataLoLimit, pCtrl->fUserDisplayDataHiLimit, (float)t);
						pControl->setValue(fValue);
						((CTextEdit*)pControl)->setText(str.c_str());
					}
				}

				break;
			}
			default:
				break;
		}

		return fValue;
	}

	inline void updateTextEdits(int nTag, CControl* pControl, CUICtrl* pCtrl)
	{
		if(!pCtrl) return;

		for(int i=0; i<m_nTextEditCount; i++)
		{
			if(m_ppTextEditControls[i]->getTag() == nTag)
			{
				m_ppTextEditControls[i]->setValueNormalized(pControl->getValueNormalized());
				switch(pCtrl->uUserDataType)
				{
					case floatData:
					{
						m_ppTextEditControls[i]->setText(floatToString(*pCtrl->m_pUserCookedFloatData,2));
						break;
					}
					case doubleData:
					{
						m_ppTextEditControls[i]->setText(doubleToString(*pCtrl->m_pUserCookedDoubleData,2));
						break;
					}
					case intData:
					{
						m_ppTextEditControls[i]->setText(intToString(*pCtrl->m_pUserCookedIntData));
						break;
					}
					case UINTData:
					{
						char* pEnum;
						pEnum = getEnumString(pCtrl->cEnumeratedList, (int)(*(pCtrl->m_pUserCookedUINTData)));
						if(pEnum)
							m_ppTextEditControls[i]->setText(pEnum);
						break;
					}
					default:
						break;
				}
			}
		}
	}

	inline float updateLCDGuts(int nLCDControlIndex)
	{
		// --- set the parameter info
		// This does the name label and the 1/3 indexCount label
		CUICtrl* pCtrl = m_pPlugIn->m_UIControlList.getAt(m_pLCDControlMap[nLCDControlIndex]);
		if(pCtrl)
		{
			string sUnits(pCtrl->cUnits);
			sUnits = trim(sUnits);
			if(sUnits.length() > 0)
			{
				string p1 = " (";
				p1.append(sUnits);
				p1.append(")");

				// -- this is	Name (units)
				string sName(pCtrl->cControlName);
				string nameUnits = sName.append(p1);
				m_pLCDControlNameLabel->setText(nameUnits.c_str());
			}
			else
                // --- name only
				m_pLCDControlNameLabel->setText(pCtrl->cControlName);

			int nIndex = nLCDControlIndex + 1;
			char* cIndex = intToString(nIndex);
			string sIndex(cIndex);

			sIndex.append("/");
			char* cCount = intToString(m_nLCDControlCount);
			sIndex.append(cCount);
			m_pLCDControlIndexCountLabel->setText(sIndex.c_str());
			delete cCount;
			delete cIndex;

		}

		// --- this is the Edit control
		CTextEdit* pControl = getLCDTextEdit(); //->setValue(pControl->getValue());
		if(pControl)
		{
			int nPlugInControlIndex = m_pLCDControlMap[m_nAlphaWheelIndex];
			pControl->setValue(m_pPlugIn->getParameter(nPlugInControlIndex));
			CUICtrl* pCtrl = m_pPlugIn->m_UIControlList.getAt(nPlugInControlIndex);
			if(pCtrl)
			{
				switch(pCtrl->uUserDataType)
				{
					case floatData:
					{
						pControl->setText(floatToString(*pCtrl->m_pUserCookedFloatData,2));
						break;
					}
					case doubleData:
					{
						pControl->setText(doubleToString(*pCtrl->m_pUserCookedDoubleData,2));
						break;
					}
					case intData:
					{
						pControl->setText(intToString(*pCtrl->m_pUserCookedIntData));
						break;
					}
					case UINTData:
					{
						char* pEnum;
						pEnum = getEnumString(pCtrl->cEnumeratedList, (int)(*(pCtrl->m_pUserCookedUINTData)));
						if(pEnum)
							pControl->setText(pEnum);
						break;
					}
					default:
						break;
				}
				pControl->invalid();
			}
		}

		// --- update the knob
		int nPlugInControlIndex = m_pLCDControlMap[m_nAlphaWheelIndex];
        float fValue = m_pPlugIn->getParameter(nPlugInControlIndex);
		getLCDValueKnob()->setValue(fValue);
		getLCDValueKnob()->invalid();

        return fValue;
	}
 	inline bool isKnobControl(CControl* pControl)
	{
		CAnimKnob* control = dynamic_cast<CAnimKnob*>(pControl);
		if(control)
			return true;

		return false;
	}

	inline bool isSliderControl(CControl* pControl)
	{
		CSlider* control = dynamic_cast<CSlider*>(pControl);
		if(control)
			return true;

		return false;
	}

	inline bool isTextEditControl(CControl* pControl)
	{
		CTextEdit* control = dynamic_cast<CTextEdit*>(pControl);
		if(control)
			return true;

		return false;
	}

	inline bool isOptionMenuControl(CControl* pControl)
	{
		COptionMenu* control = dynamic_cast<COptionMenu*>(pControl);
		if(control)
			return true;

		return false;
	}

	inline bool isRadioButtonControl(CControl* pControl)
	{
		CVerticalSwitch* control = dynamic_cast<CVerticalSwitch*>(pControl);
		if(control)
			return true;

		return false;
	}

 	inline void updateAnimKnobs(int nTag, float fValue)
	{
		for(int i=0; i<m_nAnimKnobCount; i++)
		{
			if(m_ppKnobControls[i]->getTag() == nTag)
			{
				m_ppKnobControls[i]->setValue(fValue);
				m_ppKnobControls[i]->invalid();
			}
		}
	}

   	inline float getPluginParameterValue(CUICtrl* pCtrl, float fControlValue, CControl* pControl = NULL)
	{
		if(pControl)
		{
			if(isOptionMenuControl(pControl))
				return pControl->getValue()/((float)((COptionMenu*)pControl)->getNbEntries() - 1);
			else
				return pControl->getValue();
		}
        return fControlValue; 
	}
    
	// trim from start
	static inline std::string &ltrim(std::string &s) {
        s.erase(s.begin(), std::find_if(s.begin(), s.end(), std::not1(std::ptr_fun<int, int>(std::isspace))));
        return s;
	}

	// trim from end
	static inline std::string &rtrim(std::string &s) {
        s.erase(std::find_if(s.rbegin(), s.rend(), std::not1(std::ptr_fun<int, int>(std::isspace))).base(), s.end());
        return s;
	}

	// trim from both ends
	static inline std::string &trim(std::string &s) {
        return ltrim(rtrim(s));
	}

	inline float getInitControlValue(CUICtrl* pCtrl)
	{
		if(!pCtrl)
			return 0.0;

		switch(pCtrl->uUserDataType)
		{
			case floatData:
			{
				return pCtrl->fInitUserFloatValue;
				break;
			}
			case doubleData:
			{
				return pCtrl->fInitUserDoubleValue;
				break;
			}
			case intData:
			{
				return pCtrl->fInitUserIntValue;
				break;
			}
			case UINTData:
			{
				return pCtrl->fInitUserUINTValue;
				break;
			}
			default:
				break;
		}
		return 0;
	}

	inline float getInitDisplayValue(CUICtrl* pCtrl)
	{
		return calcDisplayVariable(pCtrl->fUserDisplayDataLoLimit, pCtrl->fUserDisplayDataHiLimit, getInitControlValue(pCtrl));
	}
	inline float getInitNormalizedValue(CUICtrl* pCtrl)
	{
		return calcSliderVariable(pCtrl->fUserDisplayDataLoLimit, pCtrl->fUserDisplayDataHiLimit, getInitControlValue(pCtrl));
	}

	inline CTextEdit* getLCDEditBox(int nTag)
	{
		for(int i=0; i<m_nTextEditCount; i++)
		{
			if(m_ppTextEditControls[i]->getTag() == nTag)
				return m_ppTextEditControls[i];
		}
		return NULL;
	}

	inline void inflateRect(CRect& rect, int x, int y)
	{
		rect.left -= x;
		rect.right += x;
		rect.top -= y;
		rect.bottom += y;
	}

    // -- not needed?
	inline bool hasRafxTemplate(string templateName)
	{
		return m_XMLParser.hasRafxTemplate(templateName.c_str());
	}

	string* getUserTemplates(int& nCount);
	inline pugi::xml_node getRafxXMLNode(CView* pV)
	{
		int32_t outSize;
		pugi::xml_node node;

		pV->getAttribute(RAFX_XML_NODE, sizeof(pugi::xml_node*), &node, outSize);

		return node;
	}

	inline const char* getViewNodeAttribute(CView* pV, const char* att)
	{
		pugi::xml_node node = getRafxXMLNode(pV);

		if(node.empty())
			return NULL;

		const char* attribute = m_XMLParser.getAttribute(node, att);

		return attribute;
	}

protected:
	CPlugIn* m_pPlugIn;
    AudioUnit m_AU;
 	AUEventListenerRef AUEventListener;
   
	UINT* m_pControlMap;
	UINT* m_pLCDControlMap;
	int m_nLCDControlCount;
	int m_nAlphaWheelIndex;

	// --- arrays - remember to add to CalculateFrameSize() when you addd morearrays
	CTextLabel** m_ppTextLabels;
	int m_nTextLabelCount;

	CTextEdit** m_ppTextEditControls;
	int m_nTextEditCount;

	CAnimKnob** m_ppKnobControls;
	int m_nAnimKnobCount;

	CSlider** m_ppSliderControls;
	int m_nSliderCount;

	CKickButton** m_ppKickButtonControls;
	int m_nKickButtonCount;

	COnOffButton** m_ppOnOffButtonControls;
	int m_nOnOffButtonCount;

	CVerticalSwitch** m_ppRadioButtonControls;
	int m_nRadioButtonCount;

	CVuMeterWP** m_ppVuMeterControls;
	int m_nVuMeterCount;

	CXYPadWP** m_ppXYPads;
	int m_nXYPadCount;
	float m_fJS_X;
	float m_fJS_Y;

	COptionMenu** m_ppOptionMenuControls;
	int m_nOptionMenuCount;

	CViewContainer** m_ppViewContainers;
	int m_nViewContainerCount;
};

}

#endif // __CRafxVSTEditor__

