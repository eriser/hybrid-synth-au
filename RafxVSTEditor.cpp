#include "RafxVSTEditor.h"
#include "KnobWP.h"
#include "SliderWP.h"
#include "KickButtonWP.h"

namespace VSTGUI
{
void* gBundleRef = 0;
static int openCount = 0;
    
// --- this is called when presets change for us to synch GUI
void EventListenerDispatcher(void *inRefCon, void *inObject, const AudioUnitEvent *inEvent, UInt64 inHostTime, Float32 inValue)
{
    CRafxVSTEditor *pEditor = (CRafxVSTEditor*)inRefCon;
    
    if(pEditor)
        // --- just do a brute force init, no need to call broadcastControlChange() here
        pEditor->initControls(false);
}

#if MAC
#include <dlfcn.h>
        static void CreateVSTGUIBundleRef ();
        static void ReleaseVSTGUIBundleRef ();
#endif
    //------------------------------------------------------------------------
    void CreateVSTGUIBundleRef ()
    {
        openCount++;
        if (gBundleRef)
        {
            CFRetain (gBundleRef);
            return;
        }
#if TARGET_OS_IPHONE
        gBundleRef = CFBundleGetMainBundle ();
        CFRetain (gBundleRef);
#else
        Dl_info info;
        if (dladdr ((const void*)CreateVSTGUIBundleRef, &info))
        {
            if (info.dli_fname)
            {
                string name;
                name.assign (info.dli_fname);
                for (int i = 0; i < 3; i++)
                {
                    int delPos = name.find_last_of('/');
                    if (delPos == -1)
                    {
                        fprintf (stdout, "Could not determine bundle location.\n");
                        return; // unexpected
                    }
                    name.erase (delPos, name.length () - delPos);
                }
                CFURLRef bundleUrl = CFURLCreateFromFileSystemRepresentation (0, (const UInt8*)name.c_str(), name.length (), true);
                if (bundleUrl)
                {
                    gBundleRef = CFBundleCreate (0, bundleUrl);
                    CFRelease (bundleUrl);
                }
            }
        }
#endif
    }

    //------------------------------------------------------------------------
    void ReleaseVSTGUIBundleRef ()
    {
        openCount--;
        if (gBundleRef)
            CFRelease (gBundleRef);
        if (openCount == 0)
            gBundleRef = 0;
    }

/*
enum CKnobMode
{
	kCircularMode = 0,
	kRelativCircularMode,
	kLinearMode
};*/
int32_t CRafxVSTEditor::getKnobMode() const
{
	return m_uKnobAction;
}

void CRafxVSTEditor::getSize(float& width, float& height)
{
    CRect rect;
    frame->getViewSize(rect);
    width = rect.width();
    height = rect.height();
}

CMessageResult CRafxVSTEditor::notify(CBaseObject* /*sender*/, const char* message)
{
    if(message == CVSTGUITimer::kMsgTimer)
    {
        if(frame)
            this->idle();
        
        if(m_pPlugIn)
        {
            if(m_pPlugIn->m_uPlugInEx[3] == 1)
            {
                this->initControls();
                m_pPlugIn->m_uPlugInEx[3] = 0;
            }
        }
        
        return kMessageNotified;
    }

    return kMessageUnknown;
}

// --- Will Pirkle: this object based looselt on http://sourceforge.net/p/vstgui/mailman/message/28418790/
CRafxVSTEditor::CRafxVSTEditor()
{
    m_pPlugIn = NULL; // OK if null
    m_AU = NULL;
    m_pControlMap = NULL;
    m_pLCDControlMap = NULL;
    m_nAlphaWheelIndex = 0;
    m_bInitialized = false;

    m_ppTextEditControls = NULL;
    m_nTextEditCount = 0;

    m_ppKnobControls = NULL;
    m_nAnimKnobCount = 0;

    m_ppSliderControls = NULL;
    m_nSliderCount = 0;

    m_ppKickButtonControls = NULL;
    m_nKickButtonCount = 0;

    m_ppOnOffButtonControls = NULL;
    m_nOnOffButtonCount = 0;

    m_ppRadioButtonControls = NULL;
    m_nRadioButtonCount = 0;

    m_ppVuMeterControls = NULL;
    m_nVuMeterCount = 0;

    m_ppXYPads = NULL;
    m_nXYPadCount = 0;

    m_ppViewContainers = NULL;
    m_nViewContainerCount = 0;

    m_ppOptionMenuControls = NULL;
    m_nOptionMenuCount = 0;

    m_fJS_X = 0;
    m_fJS_Y = 0;

    m_pLCDControlNameLabel = NULL;
    m_pLCDControlIndexCountLabel = NULL;
    bClosing = false;

    m_ppTextLabels = NULL;
    m_nTextLabelCount = 0;

    // --- def
    m_uKnobAction = kLinearMode;

    // --- for VSTGUI
    CreateVSTGUIBundleRef();

   	// create a timer used for idle update: will call notify method
	timer = new CVSTGUITimer (dynamic_cast<CBaseObject*>(this));

}
CRafxVSTEditor::~CRafxVSTEditor()
{
    if(timer)
        timer->forget();

    ReleaseVSTGUIBundleRef();
    m_bInitialized = false;
}

VSTGUI::CNinePartTiledBitmap* CRafxVSTEditor::loadTiledBitmap(const CResourceDescription& desc, CCoord left, CCoord top, CCoord right, CCoord bottom)
{
    VSTGUI::CNinePartTiledBitmap* pBM = new VSTGUI::CNinePartTiledBitmap(desc, (CNinePartTiledBitmap::PartOffsets(left, top, right, bottom)));
    return pBM;
}

VSTGUI::CBitmap* CRafxVSTEditor::loadBitmap(const CResourceDescription& desc)
{
    VSTGUI::CBitmap* pBM = new VSTGUI::CBitmap(desc);
    return pBM;
}

//-----------------------------------------------------------------------------------
bool CRafxVSTEditor::open(void* pHwnd, char* pXMLFile, AudioUnit inAU)
{
    if(!pHwnd) return false;
    
    m_AU = inAU;

    if(pXMLFile)// && !m_bInitialized)
    {
        // --- open the file from path
        if(!m_XMLParser.loadXMLFile(pXMLFile))
            return false;
    }

    // --- the knob action attribute is custom
    xml_node anode;
    const char* action = m_XMLParser.getCustomAttribute("KnobAction", anode);
    if(action)
        m_uKnobAction = atoi(action);

    // --- create the frame; find the editor template
    int nEditorViewCount = 0;
    pugi::xml_node node = m_XMLParser.getTemplateInfo("Editor", nEditorViewCount);
    if(!node)
        return false;

    // --- have editor and count so start building
    //
    int w, h = 0;
    // --- get editor size
    const char_t* size = m_XMLParser.getTemplateAttribute("Editor", "size");
    string sSize(size);

    if(!getXYFromString(sSize, w, h))
        return false;

    //-- first we create the frame with a size of x, y and set the background to white
    CRect frameSize(0, 0, w, h);
    CFrame* m_RafxFrame = new CFrame(frameSize, this);
    m_RafxFrame->open(pHwnd, kNSView);

    // --- not sure if needed
    m_RafxFrame->kDirtyCallAlwaysOnMainThread = true;

    const char_t* backColor = m_XMLParser.getTemplateAttribute("Editor", "background-color");
    string sBackColorName(backColor);
    CColor backCColor;

    if(isBuiltInColor(sBackColorName, backCColor))
        m_RafxFrame->setBackgroundColor(backCColor);
    else
    {
        // do we have it?
        if(m_XMLParser.hasColor(backColor))
        {
            const char_t* rgba = m_XMLParser.getColorAttribute(backColor, "rgba");
            string sRGBA(rgba);

            int r, g, b, a = 255;
            getRGBAFromString(sRGBA, r, g, b, a);
            m_RafxFrame->setBackgroundColor(CColor(r, g, b, a));
        }
    }

    // --- background bitmap (if there is one)
    const char_t* bitmap = m_XMLParser.getTemplateAttribute("Editor", "bitmap");
    if(strlen(bitmap) > 0)
    {
        setCurrentBackBitmap(bitmap, m_RafxFrame);
    }

    // --- create arrays
    m_ppTextEditControls = new CTextEdit*[m_nTextEditCount];
    m_ppKnobControls = new CAnimKnob*[m_nAnimKnobCount];
    m_ppSliderControls = new CSlider*[m_nSliderCount];
    m_ppKickButtonControls = new CKickButton*[m_nKickButtonCount];
    m_ppOnOffButtonControls = new COnOffButton*[m_nOnOffButtonCount];
    m_ppRadioButtonControls = new CVerticalSwitch*[m_nRadioButtonCount];
    m_ppVuMeterControls = new CVuMeterWP*[m_nVuMeterCount];
    m_ppXYPads = new CXYPadWP*[m_nXYPadCount];
    m_ppOptionMenuControls = new COptionMenu*[m_nOptionMenuCount];
    m_ppViewContainers = new CViewContainer*[m_nViewContainerCount];
    m_ppTextLabels = new CTextLabel*[m_nTextLabelCount];

    // --- clear arrays
    memset(m_ppTextEditControls, 0, sizeof(CTextEdit*)*m_nTextEditCount);
    memset(m_ppKnobControls, 0, sizeof(CAnimKnob*)*m_nAnimKnobCount);
    memset(m_ppSliderControls, 0, sizeof(CSlider*)*m_nSliderCount);
    memset(m_ppKickButtonControls, 0, sizeof(CKickButton*)*m_nKickButtonCount);
    memset(m_ppOnOffButtonControls, 0, sizeof(COnOffButton*)*m_nOnOffButtonCount);
    memset(m_ppRadioButtonControls, 0, sizeof(CVerticalSwitch*)*m_nRadioButtonCount);
    memset(m_ppVuMeterControls, 0, sizeof(CVuMeterWP*)*m_nVuMeterCount);
    memset(m_ppXYPads, 0, sizeof(CXYPad*)*m_nXYPadCount);
    memset(m_ppOptionMenuControls, 0, sizeof(COptionMenu*)*m_nOptionMenuCount);
    memset(m_ppViewContainers, 0, sizeof(CViewContainer*)*m_nViewContainerCount);
    memset(m_ppTextLabels, 0, sizeof(CTextLabel*)*m_nTextLabelCount);

    // --- create subviews
    for(int i=0; i<nEditorViewCount; i++)
    {
        VIEW_DESC* pViewDesc = m_XMLParser.getTemplateSubViewDesc("Editor", i);
        if(pViewDesc)
        {
            createSubView(m_RafxFrame, pViewDesc->viewNode, true);
            delete pViewDesc;
        }
    }

    //-- set the member frame to our frame
    frame = m_RafxFrame;
    frame->enableTooltips(true);
    if(timer)
	{
        timer->setFireTime(50);
        timer->start();
    }

    m_bInitialized = true;

    return true;
}

//-----------------------------------------------------------------------------------
void CRafxVSTEditor::close()
{
    if(!frame) return;

    bClosing = true;

    // --- delete pointers BUT NOT OBJECTS; they will be deleted in frame->forget()
    // --- arrays - remember to add to CalculateFrameSize() when you addd morearrays
    if(m_ppTextLabels)
        delete [] m_ppTextLabels;

    if(m_ppTextEditControls)
        delete [] m_ppTextEditControls;

    if(m_ppKnobControls)
        delete [] m_ppKnobControls;

    if(m_ppSliderControls)
        delete [] m_ppSliderControls;

    if(m_ppKickButtonControls)
        delete [] m_ppKickButtonControls;

    if(m_ppOnOffButtonControls)
        delete [] m_ppOnOffButtonControls;

    if(m_ppRadioButtonControls)
        delete [] m_ppRadioButtonControls;

    if(m_ppVuMeterControls)
        delete [] m_ppVuMeterControls;

    if(m_ppXYPads)
        delete [] m_ppXYPads;

    if(m_ppOptionMenuControls)
        delete [] m_ppOptionMenuControls;

    if(m_ppViewContainers)
        delete [] m_ppViewContainers;

    m_nTextEditCount = 0;
	m_nAnimKnobCount = 0;
	m_nSliderCount = 0;
	m_nKickButtonCount = 0;
	m_nOnOffButtonCount = 0;
	m_nRadioButtonCount = 0;
	m_nVuMeterCount = 0;
	m_nXYPadCount = 0;
	m_nOptionMenuCount = 0;
	m_nViewContainerCount = 0;
	m_nTextLabelCount = 0;

    timer->stop();

    //-- on close we need to delete the frame object.
    //-- once again we make sure that the member frame variable is set to zero before we delete it
    //-- so that calls to setParameter won't crash.
    CFrame* oldFrame = frame;
    frame = 0;
    oldFrame->forget();
    bClosing = false;
}

void CRafxVSTEditor::idle()
{
    if(bClosing) return;

    // --- update meters
    if(m_ppVuMeterControls && m_pControlMap && m_pPlugIn)
    {
        for(int i=0; i<m_nVuMeterCount; i++)
        {
            CVuMeterWP* pControl = m_ppVuMeterControls[i];
            if(pControl)
            {
                int nControlIndex = m_XMLParser.getTagIndex(pControl->getTag());
                int nPlugInControlIndex = m_pControlMap[nControlIndex];
                CUICtrl* pUICtrl = m_pPlugIn->m_UIControlList.getAt(nPlugInControlIndex);
                if(pUICtrl)
                {
                    if(pUICtrl->m_pCurrentMeterValue)
                    {
                        float fMeter = *pUICtrl->m_pCurrentMeterValue;
                        pControl->updateMeter(fMeter);
                        pControl->invalid();
                    }
                }
            }
        }
    }

    // --- update frame - important; this updates edit boxes, sliders, etc...
    if(frame)
        frame->idle();
}

// --- only for controls embedded in the frame
bool CRafxVSTEditor::createSubView(CViewContainer* pParentView, pugi::xml_node viewNode, bool bFrameSubView)
{
    const char_t* classname = m_XMLParser.getAttribute(viewNode, "class");
    string sClassName = string(classname);

    // --- currently only supporting CViewContainer to hold sub-views
    if(sClassName == "CViewContainer")
    {
        // --- get bitmap first
        CBitmap* backBitmap = getLoadBitmap(viewNode, "bitmap");

        CViewContainer* pVC = createViewContainer(viewNode, backBitmap);
        pParentView->addView(pVC);

        // --- is this a template VC or stand-alone?
        const char_t* templatename = m_XMLParser.getAttribute(viewNode, "template");
        if(strlen(templatename) > 0)
        {
            int count = 0;
            pugi::xml_node templateNode = m_XMLParser.getTemplateInfo(templatename, count);
            for(int i=0; i<count; i++)
            {
                if(pParentView == frame)
                    createSubView(pVC, m_XMLParser.getViewSubViewNode(templateNode, i), false);
                else
                    createSubView(pVC, m_XMLParser.getViewSubViewNode(templateNode, i), true);
            }
        }
        else // I don't think this will get called ever? --  YES for ViewContainers inside of ViewConatiners
        {
            int count = m_XMLParser.getSubViewCount(viewNode);
            for(int i=0; i<count; i++)
            {
                createSubView(pVC, m_XMLParser.getViewSubViewNode(viewNode, i), true);
            }
        }

        if(backBitmap)
            backBitmap->forget();
    }

    // --- text labels:
    if(sClassName == "CTextLabel")
    {
        // ---										  addingNew, standalone)
        CTextLabel* pLabel = createTextLabel(viewNode, true, bFrameSubView); // if bFrameSubView, standAlone = true
        pParentView->addView(pLabel);
    }
    if(sClassName == "CTextEdit")
    {
        CTextEdit* pEdit = createTextEdit(viewNode, true, bFrameSubView);
        pParentView->addView(pEdit);
    }

    if(sClassName == "COnOffButton")
    {
        // --- get bitmap first
        CBitmap* buttBitmap = getLoadBitmap(viewNode, "bitmap");
        COnOffButton* pButt = createOnOffButton(viewNode, buttBitmap, true, bFrameSubView);

        // --- add it
        pParentView->addView(pButt);

        // --- forget bitmap
        if(buttBitmap)
            buttBitmap->forget();
    }

    if(sClassName == "CKickButton")
    {
        // --- get bitmap first
        CBitmap* buttBitmap = getLoadBitmap(viewNode, "bitmap");
        CKickButton* pButt = createKickButton(viewNode, buttBitmap, true, bFrameSubView);

        // --- add it
        pParentView->addView(pButt);

        // --- forget bitmap
        if(buttBitmap)
            buttBitmap->forget();
    }

    if(sClassName == "CVerticalSwitch")
    {
        // --- get bitmap first
        CBitmap* switchBitmap = getLoadBitmap(viewNode, "bitmap");
        CVerticalSwitch* pSwitch = createVerticalSwitch(viewNode, switchBitmap);

        // --- add it
        pParentView->addView(pSwitch);

        // --- forget bitmap
        if(switchBitmap)
            switchBitmap->forget();
    }

    if(sClassName == "COptionMenu")
    {
        // --- get bitmap first
        CBitmap* backBitmap = getLoadBitmap(viewNode, "bitmap");
        COptionMenu* pOM = createOptionMenu(viewNode, backBitmap, true, bFrameSubView);

        // --- add it
        pParentView->addView(pOM);

        // --- forget bitmap
        if(backBitmap)
            backBitmap->forget();
    }

    if(sClassName == "CXYPad")
    {
        // --- get bitmap first
        CBitmap* backBitmap = getLoadBitmap(viewNode, "bitmap");
        CXYPad* pPad = createXYPad(viewNode, backBitmap, true, bFrameSubView);

        // --- add it
        pParentView->addView(pPad);

        // --- forget bitmap
        if(backBitmap)
            backBitmap->forget();
    }

    if(sClassName == "CAnimKnob")
    {
        // --- get bitmap first
        CBitmap* knobBitmap = getLoadBitmap(viewNode, "bitmap");
        CAnimKnob* pKnob = createAnimKnob(viewNode, knobBitmap, true, bFrameSubView);

        // --- add it
        pParentView->addView(pKnob);

        // --- forget bitmap
        if(knobBitmap)
            knobBitmap->forget();
    }

    if(sClassName == "CSlider")
    {
        // --- get bitmap first
        CBitmap* grooveBitmap = getLoadBitmap(viewNode, "bitmap");
        CBitmap* handleBitmap = getLoadBitmap(viewNode, "handle-bitmap");

        CSlider* pSlider = createSlider(viewNode, handleBitmap, grooveBitmap, true, bFrameSubView);
        pSlider->setValue(0.0);

        // --- add it
        pParentView->addView(pSlider);
        pSlider->invalid();

        // --- forget bitmaps
        if(grooveBitmap)
            grooveBitmap->forget();
        if(handleBitmap)
            handleBitmap->forget();
    }

    if(sClassName == "CVuMeter")
    {
        // --- get bitmap first
        CBitmap* onBitmap = getLoadBitmap(viewNode, "bitmap");
        CBitmap* offBitmap = getLoadBitmap(viewNode, "off-bitmap");

        CVuMeter* pMeter = createMeter(viewNode, onBitmap, offBitmap, true, bFrameSubView);

        // --- add it
        pParentView->addView(pMeter);

        // --- forget bitmaps
        if(onBitmap)
            onBitmap->forget();
        if(offBitmap)
            offBitmap->forget();
    }
    return true;
}

CViewContainer* CRafxVSTEditor::createViewContainer(pugi::xml_node node, CBitmap* backgroundBitmap, bool bAddingNew, bool bStandAlone)
{
    CViewContainer* pVC = new CViewContainer(getRectFromNode(node));
    if(backgroundBitmap)
        pVC->setBackground(backgroundBitmap);

    const char_t* backcolor = m_XMLParser.getAttribute(node, "background-color");
    if(strlen(backcolor) > 0)
    {
        string sBackColorName(backcolor);

        // --- get the color
        CColor backCColor = getCColor(backcolor);
        pVC->setBackgroundColor(backCColor);
    }

    const char_t* backcolorDS = m_XMLParser.getAttribute(node, "background-color-draw-style");
    if(strlen(backcolorDS) > 0)
    {
        if(strcmp(backcolorDS, "filled and stroked") == 0)
            pVC->setBackgroundColorDrawStyle(kDrawFilledAndStroked);
        else if(strcmp(backcolorDS, "filled") == 0)
            pVC->setBackgroundColorDrawStyle(kDrawFilled);
        else if(strcmp(backcolorDS, "stroked") == 0)
            pVC->setBackgroundColorDrawStyle(kDrawStroked);
    }

    // --- CView
    const char_t* transp = m_XMLParser.getAttribute(node, "transparent");
    if(strlen(transp) > 0)
    {
        if(strcmp(transp, "true") == 0)
            pVC->setTransparency(true);
        else
            pVC->setTransparency(false);
    }

    const char_t* mouseE = m_XMLParser.getAttribute(node, "mouse-enabled");
    if(strlen(mouseE) > 0)
    {
        if(strcmp(mouseE, "true") == 0)
            pVC->setMouseEnabled(true);
        else
            pVC->setMouseEnabled(false);
    }

    if(bAddingNew)
    {
        // --- save control; requires destroy/create
        CViewContainer** temp = new CViewContainer*[m_nViewContainerCount + 1];
        for(int i=0; i<m_nViewContainerCount; i++)
            temp[i] = m_ppViewContainers[i];

        // --- add new one
        temp[m_nViewContainerCount] = pVC;
        m_nViewContainerCount++;

        if(m_ppViewContainers)
            delete m_ppViewContainers;
        m_ppViewContainers = temp;
    }

    return pVC;
}

CFontDesc* CRafxVSTEditor::createFontDescFromFontNode(pugi::xml_node node)
{
    const char_t* font = m_XMLParser.getAttribute(node, "font-name");
    if(strlen(font) <= 0)
        return new CFontDesc();

    CFontDesc* fontDesc = new CFontDesc(font);

    const char_t* size = m_XMLParser.getFontAttribute(font, "size");
    if(strlen(size) > 0)
    {
        string ccoord(size);
        const CCoord fontsize = ::atof(ccoord.c_str());
        fontDesc->setSize(fontsize);
    }

    const char_t* bold = m_XMLParser.getFontAttribute(font, "bold");
    if(strlen(bold) > 0)
    {
        if(strcmp(bold, "true") == 0)
            fontDesc->setStyle(fontDesc->getStyle() | kBoldFace);
        else
            fontDesc->setStyle(fontDesc->getStyle() & ~kBoldFace);
    }

    const char_t* ital = m_XMLParser.getFontAttribute(font, "italic");
    if(strlen(ital) > 0)
    {
        if(strcmp(ital, "true") == 0)
            fontDesc->setStyle(fontDesc->getStyle() | kItalicFace);
        else
            fontDesc->setStyle(fontDesc->getStyle() & ~kBoldFace);
    }

    const char_t* strike = m_XMLParser.getFontAttribute(font, "strike-through");
    if(strlen(strike) > 0)
    {
        if(strcmp(strike, "true") == 0)
            fontDesc->setStyle(fontDesc->getStyle() | kStrikethroughFace);
        else
            fontDesc->setStyle(fontDesc->getStyle() & ~kStrikethroughFace);
    }

    const char_t* underline = m_XMLParser.getFontAttribute(font, "underline");
    if(strlen(underline) > 0)
    {
        if(strcmp(underline, "true") == 0)
            fontDesc->setStyle(fontDesc->getStyle() | kUnderlineFace);
        else
            fontDesc->setStyle(fontDesc->getStyle() & ~kUnderlineFace);
    }

    return fontDesc;

}

CFontDesc* CRafxVSTEditor::createFontDesc(pugi::xml_node node)
{
    const char_t* font = m_XMLParser.getAttribute(node, "font");
    if(strlen(font) <= 0)
        return new CFontDesc();

    const char_t* fontname = m_XMLParser.getFontAttribute(font, "font-name");
    string str(fontname);
    if(strlen(fontname) <= 0)
    {
        const CFontRef builtInFont = getBuiltInFont(font);
        if(builtInFont)
        {
            return builtInFont;
        }
    }

    CFontDesc* fontDesc = new CFontDesc(fontname);

    const char_t* size = m_XMLParser.getFontAttribute(font, "size");
    if(strlen(size) > 0)
    {
        string ccoord(size);
        const CCoord fontsize = ::atof(ccoord.c_str());
        fontDesc->setSize(fontsize);
    }

    const char_t* bold = m_XMLParser.getFontAttribute(font, "bold");
    if(strlen(bold) > 0)
    {
        if(strcmp(bold, "true") == 0)
            fontDesc->setStyle(kBoldFace);
    }

    const char_t* ital = m_XMLParser.getFontAttribute(font, "italic");
    if(strlen(ital) > 0)
    {
        if(strcmp(ital, "true") == 0)
            fontDesc->setStyle(fontDesc->getStyle() | kItalicFace);
    }

    const char_t* strike = m_XMLParser.getFontAttribute(font, "strike-through");
    if(strlen(strike) > 0)
    {
        if(strcmp(strike, "true") == 0)
            fontDesc->setStyle(fontDesc->getStyle() | kStrikethroughFace);
    }

    const char_t* underline = m_XMLParser.getFontAttribute(font, "underline");
    if(strlen(underline) > 0)
    {
        if(strcmp(underline, "true") == 0)
            fontDesc->setStyle(fontDesc->getStyle() | kUnderlineFace);
    }

    return fontDesc;
}


void CRafxVSTEditor::parseTextLabel(CTextLabel* pLabel, pugi::xml_node node)
{
    const char_t* title = m_XMLParser.getAttribute(node, "title");
    if(strlen(title) > 0)
        pLabel->setText(title);

    // --- now set supported attributes
    //
    // --- CParamDisplay
    const char_t* backcolor = m_XMLParser.getAttribute(node, "back-color");
    if(strlen(backcolor) > 0)
    {
        string sBackColorName(backcolor);

        // --- get the color
        CColor backCColor = getCColor(backcolor);
        pLabel->setBackColor(backCColor);
    }

    // --- font stuff
    CFontDesc* fontDesc = createFontDesc(node);
    fontDesc->getPlatformFont();

    pLabel->setFont(fontDesc);

    const char_t* antia = m_XMLParser.getAttribute(node, "font-antialias");
    if(strlen(antia) > 0)
    {
        if(strcmp(antia, "true") == 0)
            pLabel->setAntialias(true);
        else
            pLabel->setAntialias(false);
    }

    const char_t* fcolor = m_XMLParser.getAttribute(node, "font-color");
    if(strlen(fcolor) > 0)
    {
        CColor fCColor = getCColor(fcolor);
        pLabel->setFontColor(fCColor);
    }

    const char_t* frmcolor = m_XMLParser.getAttribute(node, "frame-color");
    if(strlen(frmcolor) > 0)
    {
        CColor frmCColor = getCColor(frmcolor);
        pLabel->setFrameColor(frmCColor);
    }

    const char_t* frmwidth = m_XMLParser.getAttribute(node, "frame-width");
    if(strlen(frmwidth) > 0)
    {
        const CCoord frmWidth = ::atof(frmwidth);
        pLabel->setFrameWidth(frmWidth);
    }

    const char_t* rrr = m_XMLParser.getAttribute(node, "round-rect-radius");
    if(strlen(rrr) > 0)
    {
        const CCoord rrradius = ::atof(rrr);
        pLabel->setRoundRectRadius(rrradius);
    }

    const char_t* shadcolor = m_XMLParser.getAttribute(node, "shadow-color");
    if(strlen(shadcolor) > 0)
    {
        CColor shadCColor = getCColor(shadcolor);
        pLabel->setShadowColor(shadCColor);
    }

    const char_t* align = m_XMLParser.getAttribute(node, "text-alignment");
    if(strlen(align) > 0)
    {
        if(strcmp(align, "left") == 0)
            pLabel->setHoriAlign(kLeftText);
        else if(strcmp(align, "center") == 0)
            pLabel->setHoriAlign(kCenterText);
        else if(strcmp(align, "right") == 0)
            pLabel->setHoriAlign(kRightText);
    }

    const char_t* inset = m_XMLParser.getAttribute(node, "text-inset");
    if(strlen(inset) > 0)
    {
        const CPoint insetPt = getCPointFromString(inset);
        pLabel->setTextInset(insetPt);
    }

    // --- currently not supported
    //const char_t* precis = m_XMLParser.getAttribute(node, "value-precision");
    //pLabel->setPrecision(precis);

    // --- CControl
    const char_t* backoffset = m_XMLParser.getAttribute(node, "background-offset");
    if(strlen(backoffset) > 0)
    {
        const CPoint point = getCPointFromString(backoffset);
        pLabel->setBackOffset(point);
    }

    const char_t* minValue = m_XMLParser.getAttribute(node, "min-value");
    if(strlen(minValue) > 0)
        pLabel->setMin(::atof(minValue));

    const char_t* maxValue = m_XMLParser.getAttribute(node, "max-value");
    if(strlen(maxValue) > 0)
        pLabel->setMax(::atof(maxValue));

    const char_t* wiv = m_XMLParser.getAttribute(node, "wheel-inc-value");
    if(strlen(wiv) > 0)
        pLabel->setWheelInc(::atof(wiv));

    // --- style shit
    const char_t* sty1 = m_XMLParser.getAttribute(node, "style-3D-in");
    if(strlen(sty1) > 0)
    {
        if(strcmp(sty1, "true") == 0)
            pLabel->setStyle(pLabel->getStyle() | k3DIn);
        else
            pLabel->setStyle(pLabel->getStyle() & ~k3DIn);
    }

    const char_t* sty2 = m_XMLParser.getAttribute(node, "style-3D-out");
    if(strlen(sty2) > 0)
    {
        if(strcmp(sty2, "true") == 0)
            pLabel->setStyle(pLabel->getStyle() | k3DOut);
        else
            pLabel->setStyle(pLabel->getStyle() & ~k3DOut);
    }

    const char_t* sty3 = m_XMLParser.getAttribute(node, "style-no-draw");
    if(strlen(sty3) > 0)
    {
        if(strcmp(sty3, "true") == 0)
            pLabel->setStyle(pLabel->getStyle() | kNoDrawStyle);
        else
            pLabel->setStyle(pLabel->getStyle() & ~kNoDrawStyle);
    }

    const char_t* sty4 = m_XMLParser.getAttribute(node, "style-no-text");
    if(strlen(sty4) > 0)
    {
        if(strcmp(sty4, "true") == 0)
            pLabel->setStyle(pLabel->getStyle() | kNoTextStyle);
        else
            pLabel->setStyle(pLabel->getStyle() & ~kNoTextStyle);
    }

    const char_t* sty5 = m_XMLParser.getAttribute(node, "style-round-rect");
    if(strlen(sty5) > 0)
    {
        if(strcmp(sty5, "true") == 0)
            pLabel->setStyle(pLabel->getStyle() | kRoundRectStyle);
        else
            pLabel->setStyle(pLabel->getStyle() & ~kRoundRectStyle);
    }

    const char_t* sty6 = m_XMLParser.getAttribute(node, "style-shadow-text");
    if(strlen(sty6) > 0)
    {
        if(strcmp(sty6, "true") == 0)
            pLabel->setStyle(pLabel->getStyle() | kShadowText);
        else
            pLabel->setStyle(pLabel->getStyle() & ~kShadowText);
    }

    const char_t* sty7 = m_XMLParser.getAttribute(node, "style-no-frame");
    if(strlen(sty7) > 0)
    {
        if(strcmp(sty7, "true") == 0)
            pLabel->setStyle(pLabel->getStyle() | kNoFrame);
        else
            pLabel->setStyle(pLabel->getStyle() & ~kNoFrame);
    }

    // --- CView
    const char_t* transp = m_XMLParser.getAttribute(node, "transparent");
    if(strlen(transp) > 0)
    {
        if(strcmp(transp, "true") == 0)
            pLabel->setTransparency(true);
        else
            pLabel->setTransparency(false);
    }

    const char_t* mouseE = m_XMLParser.getAttribute(node, "mouse-enabled");
    if(strlen(mouseE) > 0)
    {
        if(strcmp(mouseE, "true") == 0)
            pLabel->setMouseEnabled(true);
        else
            pLabel->setMouseEnabled(false);
    }
}

CTextLabel* CRafxVSTEditor::createTextLabel(pugi::xml_node node, bool bAddingNew, bool bStandAlone)
{
    CTextLabel* pLabel = new CTextLabel(getRectFromNode(node)); //, title, 0, 0);

    parseTextLabel(pLabel, node);

    const char_t* subController = m_XMLParser.getAttribute(node, "sub-controller");
    if(strlen(subController) > 0)
    {
        if(strcmp(subController, "LCDControllerControlName") == 0)
            m_pLCDControlNameLabel = pLabel;
        else if(strcmp(subController, "LCDControllerIndexCount") == 0)
            m_pLCDControlIndexCountLabel = pLabel;
    }

    if(bAddingNew)
    {
        // --- save control; requires destroy/create
        CTextLabel** temp = new CTextLabel*[m_nTextLabelCount + 1];
        for(int i=0; i<m_nTextLabelCount; i++)
            temp[i] = m_ppTextLabels[i];

        // --- add new one
        temp[m_nTextLabelCount] = pLabel;
        m_nTextLabelCount++;

        if(m_ppTextLabels)
            delete m_ppTextLabels;
        m_ppTextLabels = temp;
    }

    return pLabel;
}

CTextEdit* CRafxVSTEditor::createTextEdit(pugi::xml_node node, bool bAddingNew, bool bStandAlone)
{
    // --- tag
    const char_t* ctagName = m_XMLParser.getAttribute(node, "control-tag");

    // --- zero indexed number
    const char_t* ctag = m_XMLParser.getControlTagAttribute(ctagName, "tag");
    int32_t tag = -1;
    if(strlen(ctag) > 0)
        tag = atoi(ctag);

    CTextEdit* pEdit = new CTextEdit(getRectFromNode(node), this, tag);

    // --- CTextLabel stuff (base class of TextEdit)
    parseTextLabel(pEdit, node);

    // --- CControl
    const char_t* minValue = m_XMLParser.getAttribute(node, "min-value");
    if(strlen(minValue) > 0)
        pEdit->setMin(::atof(minValue));

    const char_t* maxValue = m_XMLParser.getAttribute(node, "max-value");
    if(strlen(maxValue) > 0)
        pEdit->setMax(::atof(maxValue));

    const char_t* defValue = m_XMLParser.getAttribute(node, "default-value");
    if(strlen(defValue) > 0)
        pEdit->setDefaultValue(::atof(defValue));

    // --- tooltip
    const char_t* tooltipText = m_XMLParser.getAttribute(node, "tooltip");
    if(strlen(tooltipText) > 0)
        pEdit->setAttribute(kCViewTooltipAttribute, strlen (tooltipText)+1, tooltipText);
    // --- CTextEdit
    const char_t* itc = m_XMLParser.getAttribute(node, "immediate-text-change");
    if(strlen(itc) > 0)
    {
        if(strcmp(itc, "true") == 0)
            pEdit->setImmediateTextChange(true);
        else
            pEdit->setImmediateTextChange(false);
    }

    if(bAddingNew)
    {
        // --- save control; requires destroy/create
        CTextEdit** temp = new CTextEdit*[m_nTextEditCount + 1];
        for(int i=0; i<m_nTextEditCount; i++)
            temp[i] = m_ppTextEditControls[i];

        // --- add new one
        temp[m_nTextEditCount] = pEdit;
        m_nTextEditCount++;

        if(m_ppTextEditControls)
            delete m_ppTextEditControls;
        m_ppTextEditControls = temp;
    }

    return pEdit;

}

CXYPadWP* CRafxVSTEditor::createXYPad(pugi::xml_node node, CBitmap* backgroundBitmap, bool bAddingNew, bool bStandAlone)
{
    CXYPadWP* pPad = new CXYPadWP(getRectFromNode(node));
    pPad->setListener(this);
    if(backgroundBitmap)
        pPad->setBackground(backgroundBitmap);

    // --- tag
    const char_t* ctagName = m_XMLParser.getAttribute(node, "control-tag");

    if(strcmp(ctagName, "JOYSTICK") == 0)
        pPad->m_bIsJoystickPad = true;
    else
        pPad->m_bIsJoystickPad = false;

    // --- zero indexed number
    const char_t* ctag = m_XMLParser.getControlTagAttribute(ctagName, "tag");
    int32_t tag = -1;
    if(strlen(ctag) > 0)
        tag = atoi(ctag);

    pPad->setTag(tag);

    const char_t* ctagNameX = m_XMLParser.getAttribute(node, "control-tagX");
    const char_t* ctagX = m_XMLParser.getControlTagAttribute(ctagNameX, "tag");
    tag = -1;
    if(strlen(ctagX) > 0)
        tag = atoi(ctagX);
    else
        tag = JOYSTICK_X;

    pPad->setTagX(tag);

    const char_t* ctagNameY = m_XMLParser.getAttribute(node, "control-tagY");
    const char_t* ctagY = m_XMLParser.getControlTagAttribute(ctagNameY, "tag");
    tag = -1;
    if(strlen(ctagY) > 0)
        tag = atoi(ctagY);
    else
        tag = JOYSTICK_Y;

    pPad->setTagY(tag);

    // --- CParamDisplay
    const char_t* backcolor = m_XMLParser.getAttribute(node, "back-color");
    if(strlen(backcolor) > 0)
    {
        string sBackColorName(backcolor);

        // --- get the color
        CColor backCColor = getCColor(backcolor);
        pPad->setBackColor(backCColor);
    }

    // --- font stuff
    CFontDesc* fontDesc = createFontDesc(node);
    fontDesc->getPlatformFont();

    pPad->setFont(fontDesc);

    const char_t* antia = m_XMLParser.getAttribute(node, "font-antialias");
    if(strlen(antia) > 0)
    {
        if(strcmp(antia, "true") == 0)
            pPad->setAntialias(true);
        else
            pPad->setAntialias(false);
    }

    const char_t* fcolor = m_XMLParser.getAttribute(node, "font-color");
    if(strlen(fcolor) > 0)
    {
        CColor fCColor = getCColor(fcolor);
        pPad->setFontColor(fCColor);
    }

    const char_t* frmcolor = m_XMLParser.getAttribute(node, "frame-color");
    if(strlen(frmcolor) > 0)
    {
        CColor frmCColor = getCColor(frmcolor);
        pPad->setFrameColor(frmCColor);
    }

    const char_t* frmwidth = m_XMLParser.getAttribute(node, "frame-width");
    if(strlen(frmwidth) > 0)
    {
        const CCoord frmWidth = ::atof(frmwidth);
        pPad->setFrameWidth(frmWidth);
    }

    const char_t* rrr = m_XMLParser.getAttribute(node, "round-rect-radius");
    if(strlen(rrr) > 0)
    {
        const CCoord rrradius = ::atof(rrr);
        pPad->setRoundRectRadius(rrradius);
    }

    const char_t* shadcolor = m_XMLParser.getAttribute(node, "shadow-color");
    if(strlen(shadcolor) > 0)
    {
        CColor shadCColor = getCColor(shadcolor);
        pPad->setShadowColor(shadCColor);
    }

    // --- style shit
    const char_t* sty1 = m_XMLParser.getAttribute(node, "style-3D-in");
    if(strlen(sty1) > 0)
    {
        if(strcmp(sty1, "true") == 0)
            pPad->setStyle(pPad->getStyle() | k3DIn);
        else
            pPad->setStyle(pPad->getStyle() & ~k3DIn);
    }

    const char_t* sty2 = m_XMLParser.getAttribute(node, "style-3D-out");
    if(strlen(sty2) > 0)
    {
        if(strcmp(sty2, "true") == 0)
            pPad->setStyle(pPad->getStyle() | k3DOut);
        else
            pPad->setStyle(pPad->getStyle() & ~k3DOut);
    }

    const char_t* sty3 = m_XMLParser.getAttribute(node, "style-no-draw");
    if(strlen(sty3) > 0)
    {
        if(strcmp(sty3, "true") == 0)
            pPad->setStyle(pPad->getStyle() | kNoDrawStyle);
        else
        {
            pPad->setStyle(pPad->getStyle() & ~kNoDrawStyle);
        }
    }

    const char_t* sty4 = m_XMLParser.getAttribute(node, "style-no-text");
    if(strlen(sty4) > 0)
    {
        if(strcmp(sty4, "true") == 0)
            pPad->setStyle(pPad->getStyle() | kNoTextStyle);
        else
            pPad->setStyle(pPad->getStyle() & ~kNoTextStyle);
    }

    const char_t* sty5 = m_XMLParser.getAttribute(node, "style-round-rect");
    if(strlen(sty5) > 0)
    {
        if(strcmp(sty5, "true") == 0)
            pPad->setStyle(pPad->getStyle() | kRoundRectStyle);
        else
            pPad->setStyle(pPad->getStyle() & ~kRoundRectStyle);
    }

    const char_t* sty6 = m_XMLParser.getAttribute(node, "style-shadow-text");
    if(strlen(sty6) > 0)
    {
        if(strcmp(sty6, "true") == 0)
            pPad->setStyle(pPad->getStyle() | kShadowText);
        else
            pPad->setStyle(pPad->getStyle() & ~kShadowText);
    }

    const char_t* sty7 = m_XMLParser.getAttribute(node, "style-no-frame");
    if(strlen(sty7) > 0)
    {
        if(strcmp(sty7, "true") == 0)
            pPad->setStyle(pPad->getStyle() | kNoFrame);
        else
            pPad->setStyle(pPad->getStyle() & ~kNoFrame);
    }

    // --- CControl
    const char_t* backoffset = m_XMLParser.getAttribute(node, "background-offset");
    if(strlen(backoffset) > 0)
    {
        const CPoint point = getCPointFromString(backoffset);
        pPad->setBackOffset(point);
    }

    const char_t* align = m_XMLParser.getAttribute(node, "text-alignment");
    if(strlen(align) > 0)
    {
        if(strcmp(align, "left") == 0)
            pPad->setHoriAlign(kLeftText);
        else if(strcmp(align, "center") == 0)
            pPad->setHoriAlign(kCenterText);
        else if(strcmp(align, "right") == 0)
            pPad->setHoriAlign(kRightText);
    }

    const char_t* inset = m_XMLParser.getAttribute(node, "text-inset");
    if(strlen(inset) > 0)
    {
        const CPoint insetPt = getCPointFromString(inset);
        pPad->setTextInset(insetPt);
    }

    const char_t* wiv = m_XMLParser.getAttribute(node, "wheel-inc-value");
    if(strlen(wiv) > 0)
        pPad->setWheelInc(::atof(wiv));

    // --- CView
    const char_t* transp = m_XMLParser.getAttribute(node, "transparent");
    if(strlen(transp) > 0)
    {
        if(strcmp(transp, "true") == 0)
            pPad->setTransparency(true);
        else
            pPad->setTransparency(false);
    }

    const char_t* mouseE = m_XMLParser.getAttribute(node, "mouse-enabled");
    if(strlen(mouseE) > 0)
    {
        if(strcmp(mouseE, "true") == 0)
            pPad->setMouseEnabled(true);
        else
            pPad->setMouseEnabled(false);
    }

    pPad->invalid();

    if(bAddingNew)
    {
        // --- save control; requires destroy/create
        CXYPadWP** temp = new CXYPadWP*[m_nXYPadCount + 1];
        for(int i=0; i<m_nXYPadCount; i++)
            temp[i] = m_ppXYPads[i];

        // --- add new one
        temp[m_nXYPadCount] = pPad;
        m_nXYPadCount++;

        if(m_ppXYPads)
            delete m_ppXYPads;
        m_ppXYPads = temp;
    }

    return pPad;
}

COptionMenu* CRafxVSTEditor::createOptionMenu(pugi::xml_node node, CBitmap* backgroundBitmap, bool bAddingNew, bool bStandAlone)
{
    // --- tag
    const char_t* ctagName = m_XMLParser.getAttribute(node, "control-tag");

    // --- zero indexed number
    const char_t* ctag = m_XMLParser.getControlTagAttribute(ctagName, "tag");
    int32_t tag = -1;
    if(strlen(ctag) > 0)
        tag = atoi(ctag);

    COptionMenu* pOM = new COptionMenu(getRectFromNode(node), this, tag, backgroundBitmap, NULL);

    // --- OM specific
    const char_t* checkStyle = m_XMLParser.getAttribute(node, "menu-check-style");
    if(strlen(checkStyle) > 0)
    {
        if(strcmp(checkStyle, "true") == 0)
            pOM->setStyle(pOM->getStyle() | kCheckStyle);
        else
            pOM->setStyle(pOM->getStyle() & ~kCheckStyle);
    }

    const char_t* popupStyle = m_XMLParser.getAttribute(node, "menu-popup-style");
    if(strlen(popupStyle) > 0)
    {
        if(strcmp(popupStyle, "true") == 0)
            pOM->setStyle(pOM->getStyle() | kPopupStyle);
        else
            pOM->setStyle(pOM->getStyle() & ~kPopupStyle);
    }

    // --- CParamDisplay
    const char_t* backcolor = m_XMLParser.getAttribute(node, "back-color");
    if(strlen(backcolor) > 0)
    {
        string sBackColorName(backcolor);

        // --- get the color
        CColor backCColor = getCColor(backcolor);
        pOM->setBackColor(backCColor);
    }

    // --- font stuff
    CFontDesc* fontDesc = createFontDesc(node);
    fontDesc->getPlatformFont();

    pOM->setFont(fontDesc);

    const char_t* antia = m_XMLParser.getAttribute(node, "font-antialias");
    if(strlen(antia) > 0)
    {
        if(strcmp(antia, "true") == 0)
            pOM->setAntialias(true);
        else
            pOM->setAntialias(false);
    }

    const char_t* fcolor = m_XMLParser.getAttribute(node, "font-color");
    if(strlen(fcolor) > 0)
    {
        CColor fCColor = getCColor(fcolor);
        pOM->setFontColor(fCColor);
    }

    const char_t* frmcolor = m_XMLParser.getAttribute(node, "frame-color");
    if(strlen(frmcolor) > 0)
    {
        CColor frmCColor = getCColor(frmcolor);
        pOM->setFrameColor(frmCColor);
    }

    const char_t* frmwidth = m_XMLParser.getAttribute(node, "frame-width");
    if(strlen(frmwidth) > 0)
    {
        const CCoord frmWidth = ::atof(frmwidth);
        pOM->setFrameWidth(frmWidth);
    }

    const char_t* rrr = m_XMLParser.getAttribute(node, "round-rect-radius");
    if(strlen(rrr) > 0)
    {
        const CCoord rrradius = ::atof(rrr);
        pOM->setRoundRectRadius(rrradius);
    }

    const char_t* shadcolor = m_XMLParser.getAttribute(node, "shadow-color");
    if(strlen(shadcolor) > 0)
    {
        CColor shadCColor = getCColor(shadcolor);
        pOM->setShadowColor(shadCColor);
    }

    const char_t* align = m_XMLParser.getAttribute(node, "text-alignment");
    if(strlen(align) > 0)
    {
        if(strcmp(align, "left") == 0)
            pOM->setHoriAlign(kLeftText);
        else if(strcmp(align, "center") == 0)
            pOM->setHoriAlign(kCenterText);
        else if(strcmp(align, "right") == 0)
            pOM->setHoriAlign(kRightText);
    }

    const char_t* inset = m_XMLParser.getAttribute(node, "text-inset");
    if(strlen(inset) > 0)
    {
        const CPoint insetPt = getCPointFromString(inset);
        pOM->setTextInset(insetPt);
    }

    // --- currently not supported
    //const char_t* precis = m_XMLParser.getAttribute(node, "value-precision");
    //pLabel->setPrecision(precis);

    // --- CControl
    const char_t* backoffset = m_XMLParser.getAttribute(node, "background-offset");
    if(strlen(backoffset) > 0)
    {
        const CPoint point = getCPointFromString(backoffset);
        pOM->setBackOffset(point);
    }

    const char_t* minValue = m_XMLParser.getAttribute(node, "min-value");
    if(strlen(minValue) > 0)
        pOM->setMin(::atof(minValue));

    const char_t* maxValue = m_XMLParser.getAttribute(node, "max-value");
    if(strlen(maxValue) > 0)
        pOM->setMax(::atof(maxValue));

    const char_t* defValue = m_XMLParser.getAttribute(node, "default-value");
    if(strlen(defValue) > 0)
        pOM->setDefaultValue(::atof(defValue));

    const char_t* wiv = m_XMLParser.getAttribute(node, "wheel-inc-value");
    if(strlen(wiv) > 0)
        pOM->setWheelInc(::atof(wiv));

    // --- style shit
    const char_t* sty1 = m_XMLParser.getAttribute(node, "style-3D-in");
    if(strlen(sty1) > 0)
    {
        if(strcmp(sty1, "true") == 0)
            pOM->setStyle(pOM->getStyle() | k3DIn);
        else
            pOM->setStyle(pOM->getStyle() & ~k3DIn);
    }

    const char_t* sty2 = m_XMLParser.getAttribute(node, "style-3D-out");
    if(strlen(sty2) > 0)
    {
        if(strcmp(sty2, "true") == 0)
            pOM->setStyle(pOM->getStyle() | k3DOut);
        else
            pOM->setStyle(pOM->getStyle() & ~k3DOut);
    }

    const char_t* sty3 = m_XMLParser.getAttribute(node, "style-no-draw");
    if(strlen(sty3) > 0)
    {
        if(strcmp(sty3, "true") == 0)
            pOM->setStyle(pOM->getStyle() | kNoDrawStyle);
        else
            pOM->setStyle(pOM->getStyle() & ~kNoDrawStyle);
    }

    const char_t* sty4 = m_XMLParser.getAttribute(node, "style-no-text");
    if(strlen(sty4) > 0)
    {
        if(strcmp(sty4, "true") == 0)
            pOM->setStyle(pOM->getStyle() | kNoTextStyle);
        else
            pOM->setStyle(pOM->getStyle() & ~kNoTextStyle);
    }

    const char_t* sty5 = m_XMLParser.getAttribute(node, "style-round-rect");
    if(strlen(sty5) > 0)
    {
        if(strcmp(sty5, "true") == 0)
            pOM->setStyle(pOM->getStyle() | kRoundRectStyle);
        else
            pOM->setStyle(pOM->getStyle() & ~kRoundRectStyle);
    }

    const char_t* sty6 = m_XMLParser.getAttribute(node, "style-shadow-text");
    if(strlen(sty6) > 0)
    {
        if(strcmp(sty6, "true") == 0)
            pOM->setStyle(pOM->getStyle() | kShadowText);
        else
            pOM->setStyle(pOM->getStyle() & ~kShadowText);
    }

    const char_t* sty7 = m_XMLParser.getAttribute(node, "style-no-frame");
    if(strlen(sty7) > 0)
    {
        if(strcmp(sty7, "true") == 0)
            pOM->setStyle(pOM->getStyle() | kNoFrame);
        else
            pOM->setStyle(pOM->getStyle() & ~kNoFrame);
    }

    const char_t* tit = m_XMLParser.getAttribute(node, "title");
    if(strlen(tit) > 0)
    {
        pOM->addEntry(tit, 0);
        pOM->setCurrent(0);
    }


    // --- CView
    const char_t* transp = m_XMLParser.getAttribute(node, "transparent");
    if(strlen(transp) > 0)
    {
        if(strcmp(transp, "true") == 0)
            pOM->setTransparency(true);
        else
            pOM->setTransparency(false);
    }

    const char_t* mouseE = m_XMLParser.getAttribute(node, "mouse-enabled");
    if(strlen(mouseE) > 0)
    {
        if(strcmp(mouseE, "true") == 0)
            pOM->setMouseEnabled(true);
        else
            pOM->setMouseEnabled(false);
    }

    if(bAddingNew)
    {
        // --- save control; requires destroy/create
        COptionMenu** temp = new COptionMenu*[m_nOptionMenuCount + 1];
        for(int i=0; i<m_nOptionMenuCount; i++)
            temp[i] = m_ppOptionMenuControls[i];

        // --- add new one
        temp[m_nOptionMenuCount] = pOM;
        m_nOptionMenuCount++;

        if(m_ppOptionMenuControls)
            delete m_ppOptionMenuControls;
        m_ppOptionMenuControls = temp;
    }

    return pOM;
}

CKickButton* CRafxVSTEditor::createKickButton(pugi::xml_node node, CBitmap* backgroundBitmap, bool bAddingNew, bool bStandAlone)
{
    // --- tag
    const char_t* ctagName = m_XMLParser.getAttribute(node, "control-tag");

    // --- zero indexed number
    const char_t* ctag = m_XMLParser.getControlTagAttribute(ctagName, "tag");
    int32_t tag = -1;
    if(strlen(ctag) > 0)
        tag = atoi(ctag);

    const char_t* htOneImage = m_XMLParser.getAttribute(node, "height-of-one-image");
    CCoord htImage = ::atof(htOneImage);

    const char_t* backoffset = m_XMLParser.getAttribute(node, "background-offset");
    const CPoint point = getCPointFromString(backoffset);

    CKickButton* pButt = new CKickButtonWP(getRectFromNode(node), this, tag, backgroundBitmap, point);

    const char_t* subPix = m_XMLParser.getAttribute(node, "sub-pixmaps");
    if(strlen(subPix) > 0)
        pButt->setNumSubPixmaps(atoi(subPix));

    // --- CControl
    const char_t* minValue = m_XMLParser.getAttribute(node, "min-value");
    if(strlen(minValue) > 0)
        pButt->setMin(::atof(minValue));

    const char_t* maxValue = m_XMLParser.getAttribute(node, "max-value");
    if(strlen(maxValue) > 0)
        pButt->setMax(::atof(maxValue));

    const char_t* defValue = m_XMLParser.getAttribute(node, "default-value");
    if(strlen(defValue) > 0)
        pButt->setDefaultValue(::atof(defValue));

    const char_t* wiv = m_XMLParser.getAttribute(node, "wheel-inc-value");
    if(strlen(wiv) > 0)
        pButt->setWheelInc(::atof(wiv));
    // --- CView
    const char_t* transp = m_XMLParser.getAttribute(node, "transparent");
    if(strlen(transp) > 0)
    {
        if(strcmp(transp, "true") == 0)
            pButt->setTransparency(true);
        else
            pButt->setTransparency(false);
    }

    const char_t* mouseE = m_XMLParser.getAttribute(node, "mouse-enabled");
    if(strlen(mouseE) > 0)
    {
        if(strcmp(mouseE, "true") == 0)
            pButt->setMouseEnabled(true);
        else
            pButt->setMouseEnabled(false);
    }

    if(bAddingNew)
    {
        // --- save control; requires destroy/create
        CKickButton** temp = new CKickButton*[m_nKickButtonCount + 1];
        for(int i=0; i<m_nKickButtonCount; i++)
            temp[i] = m_ppKickButtonControls[i];

        // --- add new one
        temp[m_nKickButtonCount] = pButt;
        m_nKickButtonCount++;

        if(m_ppKickButtonControls)
            delete m_ppKickButtonControls;
        m_ppKickButtonControls = temp;
    }

    return pButt;
}

CVuMeterWP* CRafxVSTEditor::createMeter(pugi::xml_node node, CBitmap* onBitmap, CBitmap* offBitmap, bool bAddingNew, bool bStandAlone)
{
    const char_t* numLED = m_XMLParser.getAttribute(node, "num-led");
    int32_t nbLed = -1;
    if(strlen(numLED) > 0)
        nbLed = atoi(numLED);

    const char_t* orient = m_XMLParser.getAttribute(node, "orientation");

    // --- inverted meters! YIKES How to handle this?? May need derived class...
    const char_t* custView = m_XMLParser.getAttribute(node, "custom-view-name");
    bool bInverted  = false;
    bool bAnalogVU  = false;
    if(strlen(custView) > 0)
    {
        if(strcmp(custView, "InvertedMeterView") == 0)
        {
            bInverted = true;
            bAnalogVU = false;
        }
        else if(strcmp(custView, "InvertedAnalogMeterView") == 0)
        {
            bInverted = true;
            bAnalogVU = true;
        }
        else if(strcmp(custView, "AnalogMeterView") == 0)
        {
            bInverted = false;
            bAnalogVU = true;
        }
    }

    CVuMeterWP* pMeter;
    if(strcmp(orient, "vertical") == 0)
        pMeter = new CVuMeterWP(getRectFromNode(node), onBitmap, offBitmap, nbLed, bInverted, bAnalogVU, kVertical);
    else
        pMeter = new CVuMeterWP(getRectFromNode(node), onBitmap, offBitmap, nbLed, bInverted, bAnalogVU, kHorizontal);

    // --- wp special for VU METERS!
    const char_t* htoi = m_XMLParser.getAttribute(node, "height-of-one-image");
    if(strlen(htoi) > 0)
    {
        pMeter->setHtOneImage(atof(htoi));
    }

    const char_t* spm = m_XMLParser.getAttribute(node, "sub-pixmaps");
    if(strlen(spm) > 0)
    {
        pMeter->setImageCount(atof(spm));
    }

    const char_t* zdbf = m_XMLParser.getAttribute(node, "zero-dB-Frame");
    if(strlen(zdbf) > 0)
    {
        pMeter->setZero_dB_Frame(atof(zdbf));
    }

    // --- CControl
    const char_t* minValue = m_XMLParser.getAttribute(node, "min-value");
    if(strlen(minValue) > 0)
        pMeter->setMin(::atof(minValue));

    const char_t* maxValue = m_XMLParser.getAttribute(node, "max-value");
    if(strlen(maxValue) > 0)
        pMeter->setMax(::atof(maxValue));

    const char_t* defValue = m_XMLParser.getAttribute(node, "default-value");
    if(strlen(defValue) > 0)
        pMeter->setDefaultValue(::atof(defValue));

    const char_t* backoffset = m_XMLParser.getAttribute(node, "background-offset");
    if(strlen(backoffset) > 0)
    {
        const CPoint point = getCPointFromString(backoffset);
        pMeter->setBackOffset(point);
    }

    // --- tag
    const char_t* ctagName = m_XMLParser.getAttribute(node, "control-tag");
    if(strlen(ctagName) > 0)
    {
        // --- zero indexed number
        const char_t* ctag = m_XMLParser.getControlTagAttribute(ctagName, "tag");
        int32_t tag = -1;
        if(strlen(ctag) > 0)
            tag = atoi(ctag);

        pMeter->setTag(tag);
    }

    const char_t* wiv = m_XMLParser.getAttribute(node, "wheel-inc-value");
    if(strlen(wiv) > 0)
        pMeter->setWheelInc(::atof(wiv));
    // --- CView
    const char_t* transp = m_XMLParser.getAttribute(node, "transparent");
    if(strlen(transp) > 0)
    {
        if(strcmp(transp, "true") == 0)
            pMeter->setTransparency(true);
        else
            pMeter->setTransparency(false);
    }

    const char_t* mouseE = m_XMLParser.getAttribute(node, "mouse-enabled");
    if(strlen(mouseE) > 0)
    {
        if(strcmp(mouseE, "true") == 0)
            pMeter->setMouseEnabled(true);
        else
            pMeter->setMouseEnabled(false);
    }

    const char_t* stepVal = m_XMLParser.getAttribute(node, "decrease-step-value");
    if(strlen(stepVal) > 0)
        pMeter->setDecreaseStepValue(atof(stepVal));

    if(bAddingNew)
    {
        // --- save control; requires destroy/create
        CVuMeterWP** temp = new CVuMeterWP*[m_nVuMeterCount + 1];
        for(int i=0; i<m_nVuMeterCount; i++)
            temp[i] = m_ppVuMeterControls[i];

        // --- add new one
        temp[m_nVuMeterCount] = pMeter;
        m_nVuMeterCount++;

        if(m_ppVuMeterControls)
            delete m_ppVuMeterControls;
        m_ppVuMeterControls = temp;
    }

    return pMeter;
}

COnOffButton* CRafxVSTEditor::createOnOffButton(pugi::xml_node node, CBitmap* bitmap, bool bAddingNew, bool bStandAlone)
{
    // --- tag
    const char_t* ctagName = m_XMLParser.getAttribute(node, "control-tag");

    // --- zero indexed number
    const char_t* ctag = m_XMLParser.getControlTagAttribute(ctagName, "tag");
    int32_t tag = -1;
    if(strlen(ctag) > 0)
        tag = atoi(ctag);

    COnOffButton* pButt = new COnOffButton(getRectFromNode(node), this, tag, bitmap);

    const char_t* backoffset = m_XMLParser.getAttribute(node, "background-offset");
    if(strlen(backoffset) > 0)
    {
        const CPoint point = getCPointFromString(backoffset);
        pButt->setBackOffset(point);
    }

    // --- CControl
    const char_t* minValue = m_XMLParser.getAttribute(node, "min-value");
    if(strlen(minValue) > 0)
        pButt->setMin(::atof(minValue));

    const char_t* maxValue = m_XMLParser.getAttribute(node, "max-value");
    if(strlen(maxValue) > 0)
        pButt->setMax(::atof(maxValue));

    const char_t* defValue = m_XMLParser.getAttribute(node, "default-value");
    if(strlen(defValue) > 0)
        pButt->setDefaultValue(::atof(defValue));

    const char_t* wiv = m_XMLParser.getAttribute(node, "wheel-inc-value");
    if(strlen(wiv) > 0)
        pButt->setWheelInc(::atof(wiv));
    // --- CView
    const char_t* transp = m_XMLParser.getAttribute(node, "transparent");
    if(strlen(transp) > 0)
    {
        if(strcmp(transp, "true") == 0)
            pButt->setTransparency(true);
        else
            pButt->setTransparency(false);
    }

    const char_t* mouseE = m_XMLParser.getAttribute(node, "mouse-enabled");
    if(strlen(mouseE) > 0)
    {
        if(strcmp(mouseE, "true") == 0)
            pButt->setMouseEnabled(true);
        else
            pButt->setMouseEnabled(false);
    }

    if(bAddingNew)
    {
        // --- save control; requires destroy/create
        COnOffButton** temp = new COnOffButton*[m_nOnOffButtonCount + 1];
        for(int i=0; i<m_nOnOffButtonCount; i++)
            temp[i] = m_ppOnOffButtonControls[i];

        // --- add new one
        temp[m_nOnOffButtonCount] = pButt;
        m_nOnOffButtonCount++;

        if(m_ppOnOffButtonControls)
            delete m_ppOnOffButtonControls;
        m_ppOnOffButtonControls = temp;
    }

    return pButt;
}

CVerticalSwitch* CRafxVSTEditor::createVerticalSwitch(pugi::xml_node node, CBitmap* backgroundBitmap, bool bAddingNew, bool bStandAlone)
{
    // --- tag
    const char_t* ctagName = m_XMLParser.getAttribute(node, "control-tag");

    // --- zero indexed number
    const char_t* ctag = m_XMLParser.getControlTagAttribute(ctagName, "tag");
    int32_t tag = -1;
    if(strlen(ctag) > 0)
        tag = atoi(ctag);

    const char_t* backoffset = m_XMLParser.getAttribute(node, "background-offset");
    const CPoint point = getCPointFromString(backoffset);

    const char_t* subpix = m_XMLParser.getAttribute(node, "sub-pixmaps");
    int32_t subPixmaps = 0;
    if(strlen(subpix) > 0)
        subPixmaps = atoi(subpix);

    const char_t* htOneImage = m_XMLParser.getAttribute(node, "height-of-one-image");
    CCoord htImage = ::atof(htOneImage);

    // NOTE: iMaxPositions is not supported on VSTGUI end of things, def to 1
    CVerticalSwitch* pSwitch = new CVerticalSwitch(getRectFromNode(node), this, tag, subPixmaps, htImage, 1, backgroundBitmap, point);

    // --- CControl
    const char_t* minValue = m_XMLParser.getAttribute(node, "min-value");
    if(strlen(minValue) > 0)
        pSwitch->setMin(::atof(minValue));

    const char_t* maxValue = m_XMLParser.getAttribute(node, "max-value");
    if(strlen(maxValue) > 0)
        pSwitch->setMax(::atof(maxValue));

    const char_t* defValue = m_XMLParser.getAttribute(node, "default-value");
    if(strlen(defValue) > 0)
        pSwitch->setDefaultValue(::atof(defValue));

    const char_t* wiv = m_XMLParser.getAttribute(node, "wheel-inc-value");
    if(strlen(wiv) > 0)
        pSwitch->setWheelInc(::atof(wiv));
    // --- CView
    const char_t* transp = m_XMLParser.getAttribute(node, "transparent");
    if(strlen(transp) > 0)
    {
        if(strcmp(transp, "true") == 0)
            pSwitch->setTransparency(true);
        else
            pSwitch->setTransparency(false);
    }

    const char_t* mouseE = m_XMLParser.getAttribute(node, "mouse-enabled");
    if(strlen(mouseE) > 0)
    {
        if(strcmp(mouseE, "true") == 0)
            pSwitch->setMouseEnabled(true);
        else
            pSwitch->setMouseEnabled(false);
    }

    if(bAddingNew)
    {
        // --- save control; requires destroy/create
        CVerticalSwitch** temp = new CVerticalSwitch*[m_nRadioButtonCount + 1];
        for(int i=0; i<m_nRadioButtonCount; i++)
            temp[i] = m_ppRadioButtonControls[i];

        // --- add new one
        temp[m_nRadioButtonCount] = pSwitch;
        m_nRadioButtonCount++;

        if(m_ppRadioButtonControls)
            delete m_ppRadioButtonControls;
        m_ppRadioButtonControls = temp;
    }

    return pSwitch;
}

CSlider* CRafxVSTEditor::createSlider(pugi::xml_node node, CBitmap* handleBitmap, CBitmap* grooveBitmap, bool bAddingNew, bool bStandAlone)
{
    // --- tag
    const char_t* ctagName = m_XMLParser.getAttribute(node, "control-tag");

    // --- zero indexed number
    const char_t* ctag = m_XMLParser.getControlTagAttribute(ctagName, "tag");
    int32_t tag = -1;
    if(strlen(ctag) > 0)
        tag = atoi(ctag);

    const char_t* minValue = m_XMLParser.getAttribute(node, "min-value");
    const char_t* maxValue = m_XMLParser.getAttribute(node, "max-value");
    const char_t* orient = m_XMLParser.getAttribute(node, "orientation");
    //const char_t* backoffset = m_XMLParser.getAttribute(node, "background-offset");
    const char_t* backoffset = m_XMLParser.getAttribute(node, "handle-offset");
    const CPoint point = getCPointFromString(backoffset);

    const char_t* sizee = m_XMLParser.getAttribute(node, "size");
    string sSizee(sizee);
    int w, nSlotHeight =0;
    getXYFromString(sSizee, w, nSlotHeight);

    CSlider* pSlider;
    double handleHt = handleBitmap->getHeight();
	const char_t* customView = m_XMLParser.getAttribute(node, "custom-view-name");

	if(strcmp(orient, "vertical") == 0)
	{
		pSlider = new CVerticalSliderWP(getRectFromNode(node), this, tag, handleHt, nSlotHeight + 2, handleBitmap, grooveBitmap, point, kBottom);
		if(strcmp(customView, "SliderSwitchView") == 0)
			((CVerticalSliderWP*)pSlider)->setSwitchSlider(true);
	}
	else
	{
		pSlider = new CHorizontalSliderWP(getRectFromNode(node), this, tag, atoi(minValue), atoi(maxValue), handleBitmap, grooveBitmap, point, kLeft);
		if(strcmp(customView, "SliderSwitchView") == 0)
			((CHorizontalSliderWP*)pSlider)->setSwitchSlider(true);
	}

    // --- CSlider
    const char_t* bitmapOffset = m_XMLParser.getAttribute(node, "bitmap-offset");
    if(strlen(bitmapOffset) > 0)
    {
        const CPoint pointBMO = getCPointFromString(bitmapOffset);
        pSlider->setOffset(pointBMO);
    }

    const char_t* handleOffset = m_XMLParser.getAttribute(node, "handle-offset");
    if(strlen(handleOffset) > 0)
    {
        const CPoint pointHO = getCPointFromString(handleOffset);
        pSlider->setOffsetHandle(pointHO);
    }

    const char_t* transHandle = m_XMLParser.getAttribute(node, "transparent-handle");
    if(strlen(transHandle) > 0)
    {
        if(strcmp(transHandle, "true") == 0)
            pSlider->setDrawTransparentHandle(true);
        else
            pSlider->setDrawTransparentHandle(false);
    }

    const char_t* mode = m_XMLParser.getAttribute(node, "mode");
    if(strlen(mode) > 0)
    {
        if(strcmp(mode, "free click") == 0)
            pSlider->setMode(VSTGUI::CSlider::kFreeClickMode);
        else if(strcmp(mode, "touch") == 0)
            pSlider->setMode(VSTGUI::CSlider::kTouchMode);
        else if(strcmp(mode, "relative touch") == 0)
            pSlider->setMode(VSTGUI::CSlider::kRelativeTouchMode);
    }

    // --- I don't know how to handle this one - can't find it's home
    const char_t* revOrient = m_XMLParser.getAttribute(node, "reverse-orientation");
    // --- add...

    const char_t* frameColor = m_XMLParser.getAttribute(node, "draw-frame-color");
    if(strlen(frameColor) > 0)
    {
        CColor frameCColor = getCColor(frameColor);
        pSlider->setFrameColor(frameCColor);
    }

    const char_t* backColor = m_XMLParser.getAttribute(node, "draw-back-color");
    if(strlen(backColor) > 0)
    {
        CColor backCColor = getCColor(backColor);
        pSlider->setBackColor(backCColor);
    }

    const char_t* valColor = m_XMLParser.getAttribute(node, "draw-value-color");
    if(strlen(valColor) > 0)
    {
        CColor valCColor = getCColor(valColor);
        pSlider->setValueColor(valCColor);
    }

    const char_t* drawBack = m_XMLParser.getAttribute(node, "draw-back");
    if(strlen(drawBack) > 0)
    {
        if(strcmp(drawBack, "true") == 0)
            pSlider->setDrawStyle(pSlider->getDrawStyle() | VSTGUI::CSlider::kDrawBack);
    }

    const char_t* drawFrame = m_XMLParser.getAttribute(node, "draw-frame");
    if(strlen(drawFrame) > 0)
    {
        if(strcmp(drawFrame, "true") == 0)
            pSlider->setDrawStyle(pSlider->getDrawStyle() | VSTGUI::CSlider::kDrawFrame);
    }

    const char_t* drawVal = m_XMLParser.getAttribute(node, "draw-value");
    if(strlen(drawVal) > 0)
    {
        if(strcmp(drawVal, "true") == 0)
            pSlider->setDrawStyle(pSlider->getDrawStyle() | VSTGUI::CSlider::kDrawValue);
    }

    const char_t* drawValCent = m_XMLParser.getAttribute(node, "draw-value-from-center");
    if(strlen(drawValCent) > 0)
    {
        if(strcmp(drawValCent, "true") == 0)
            pSlider->setDrawStyle(pSlider->getDrawStyle() | VSTGUI::CSlider::kDrawValueFromCenter);
    }

    const char_t* drawValInv = m_XMLParser.getAttribute(node, "draw-value-inverted");
    if(strlen(drawValInv) > 0)
    {
        if(strcmp(drawValInv, "true") == 0)
            pSlider->setDrawStyle(pSlider->getDrawStyle() | VSTGUI::CSlider::kDrawInverted);
    }

    const char_t* zf = m_XMLParser.getAttribute(node, "zoom-factor");
    if(strlen(zf) > 0)
        pSlider->setZoomFactor(atof(zf));

    const char_t* wiv = m_XMLParser.getAttribute(node, "wheel-inc-value");
    if(strlen(wiv) > 0)
        pSlider->setWheelInc(::atof(wiv));
    // --- CControl
    const char_t* defValue = m_XMLParser.getAttribute(node, "default-value");
    if(strlen(defValue) > 0)
        pSlider->setDefaultValue(::atof(defValue));

    const char_t* tooltipText = m_XMLParser.getAttribute(node, "tooltip");
    if(strlen(tooltipText) > 0)
        pSlider->setAttribute(kCViewTooltipAttribute, strlen (tooltipText)+1, tooltipText);
    // --- CView
    const char_t* transp = m_XMLParser.getAttribute(node, "transparent");
    if(strlen(transp) > 0)
    {
        if(strcmp(transp, "true") == 0)
            pSlider->setTransparency(true);
        else
            pSlider->setTransparency(false);
    }

    const char_t* mouseE = m_XMLParser.getAttribute(node, "mouse-enabled");
    if(strlen(mouseE) > 0)
    {
        if(strcmp(mouseE, "true") == 0)
            pSlider->setMouseEnabled(true);
        else
            pSlider->setMouseEnabled(false);
    }

    if(bAddingNew)
    {
        // --- save control; requires destroy/create
        CSlider** temp = new CSlider*[m_nSliderCount + 1];
        for(int i=0; i<m_nSliderCount; i++)
            temp[i] = m_ppSliderControls[i];

        // --- add new one
        temp[m_nSliderCount] = pSlider;
        m_nSliderCount++;

        if(m_ppSliderControls)
            delete m_ppSliderControls;
        m_ppSliderControls = temp;
    }

    return pSlider;
}

CAnimKnob* CRafxVSTEditor::createAnimKnob(pugi::xml_node node, CBitmap* bitmap, bool bAddingNew, bool bStandAlone)
{
    // --- tag
    const char_t* ctagName = m_XMLParser.getAttribute(node, "control-tag");

    // --- zero indexed number
    const char_t* ctag = m_XMLParser.getControlTagAttribute(ctagName, "tag");
    int32_t tag = -1;
    if(strlen(ctag) > 0)
        tag = atoi(ctag);

    const char_t* subpix = m_XMLParser.getAttribute(node, "sub-pixmaps");
    int32_t subPixmaps = 0;
    if(strlen(subpix) > 0)
        subPixmaps = atoi(subpix);

    const char_t* htOneImage = m_XMLParser.getAttribute(node, "height-of-one-image");
    CCoord htImage = ::atof(htOneImage);

    const char_t* backoffset = m_XMLParser.getAttribute(node, "background-offset");
    const CPoint point = getCPointFromString(backoffset);

    const char_t* customView = m_XMLParser.getAttribute(node, "custom-view-name");
	bool bSwitch = false;

	if(strcmp(customView, "KnobSwitchView") == 0)
		bSwitch = true;

	CAnimKnob* pKnob = new CKnobWP(getRectFromNode(node), this, tag, subPixmaps, htImage, bitmap, point, bSwitch);

    // --- CKnob
    const char_t* arange = m_XMLParser.getAttribute(node, "angle-range");
    if(strlen(arange) > 0)
        pKnob->setRangeAngle(k2PI*atof(arange)/360.0);

    const char_t* astart = m_XMLParser.getAttribute(node, "angle-start");
    if(strlen(astart) > 0)
        pKnob->setStartAngle(k2PI*atof(astart)/360.0);

    const char_t* zf = m_XMLParser.getAttribute(node, "zoom-factor");
    if(strlen(zf) > 0)
        pKnob->setZoomFactor(atof(zf));

    const char_t* inset = m_XMLParser.getAttribute(node, "value-inset");
    if(strlen(inset) > 0)
        pKnob->setInsetValue(::atof(inset));

    const char_t* coronaColor = m_XMLParser.getAttribute(node, "corona-color");
    if(strlen(coronaColor) > 0)
    {
        CColor coronaCColor = getCColor(coronaColor);
        pKnob->setCoronaColor(coronaCColor);
    }

    const char_t* cdd = m_XMLParser.getAttribute(node, "corona-dash-dot");
    if(strlen(cdd) > 0)
    {
        if(strcmp(cdd, "true") == 0)
            pKnob->setDrawStyle(pKnob->getDrawStyle() | VSTGUI::CKnob::kCoronaLineDashDot);
    }

    const char_t* cdraw = m_XMLParser.getAttribute(node, "corona-drawing");
    if(strlen(cdraw) > 0)
    {
        if(strcmp(cdraw, "true") == 0)
            pKnob->setDrawStyle(pKnob->getDrawStyle() | VSTGUI::CKnob::kCoronaDrawing);
    }

    const char_t* cfc = m_XMLParser.getAttribute(node, "corona-from-center");
    if(strlen(cdraw) > 0)
    {
        if(strcmp(cfc, "true") == 0)
            pKnob->setDrawStyle(pKnob->getDrawStyle() | VSTGUI::CKnob::kCoronaFromCenter);
    }

    const char_t* cinv = m_XMLParser.getAttribute(node, "corona-inverted");
    if(strlen(cinv) > 0)
    {
        if(strcmp(cinv, "true") == 0)
            pKnob->setDrawStyle(pKnob->getDrawStyle() | VSTGUI::CKnob::kCoronaInverted);
    }

    const char_t* coutline = m_XMLParser.getAttribute(node, "corona-outline");
    if(strlen(coutline) > 0)
    {
        if(strcmp(coutline, "true") == 0)
            pKnob->setDrawStyle(pKnob->getDrawStyle() | VSTGUI::CKnob::kCoronaOutline);
    }

    const char_t* cdrawing = m_XMLParser.getAttribute(node, "circle-drawing");
    if(strlen(cdrawing) > 0)
    {
        if(strcmp(cdrawing, "true") == 0)
            pKnob->setDrawStyle(pKnob->getDrawStyle() | VSTGUI::CKnob::kCoronaDrawing);
    }

    const char_t* corInset = m_XMLParser.getAttribute(node, "corona-inset");
    if(strlen(corInset) > 0)
        pKnob->setCoronaInset(::atof(corInset));


    // --- CControl
    //const char_t* minValue = m_XMLParser.getAttribute(node, "min-value");
    //pKnob->setMin(::atof(minValue));
    pKnob->setMin(0.0);

    //const char_t* maxValue = m_XMLParser.getAttribute(node, "max-value");
    //pKnob->setMax(::atof(maxValue));
    pKnob->setMax(1.0);

    const char_t* defValue = m_XMLParser.getAttribute(node, "default-value");
    if(strlen(defValue) > 0)
        pKnob->setDefaultValue(::atof(defValue));

    const char_t* Hcolor = m_XMLParser.getAttribute(node, "handle-color");
    if(strlen(Hcolor) > 0)
    {
        CColor HCColor = getCColor(Hcolor);
        pKnob->setColorHandle(HCColor);
    }

    const char_t* HScolor = m_XMLParser.getAttribute(node, "handle-shadow-color");
    if(strlen(HScolor) > 0)
    {
        CColor HSCColor = getCColor(HScolor);
        pKnob->setColorShadowHandle(HSCColor);
    }

    const char_t* handlWid = m_XMLParser.getAttribute(node, "handle-line-width");
    if(strlen(handlWid) > 0)
    {
        CCoord handlWidCoord = ::atof(handlWid);
        pKnob->setHandleLineWidth(handlWidCoord);
    }

    const char_t* wiv = m_XMLParser.getAttribute(node, "wheel-inc-value");
    if(strlen(wiv) > 0)
        pKnob->setWheelInc(::atof(wiv));

    const char_t* tooltipText = m_XMLParser.getAttribute(node, "tooltip");
    if(strlen(tooltipText) > 0)
        pKnob->setAttribute(kCViewTooltipAttribute, strlen (tooltipText)+1, tooltipText);
    // --- CView
    const char_t* transp = m_XMLParser.getAttribute(node, "transparent");
    if(strlen(transp) > 0)
    {
        if(strcmp(transp, "true") == 0)
            pKnob->setTransparency(true);
        else
            pKnob->setTransparency(false);
    }

    const char_t* mouseE = m_XMLParser.getAttribute(node, "mouse-enabled");
    if(strlen(mouseE) > 0)
    {
        if(strcmp(mouseE, "true") == 0)
            pKnob->setMouseEnabled(true);
        else
            pKnob->setMouseEnabled(false);
    }

    if(bAddingNew)
    {
        // --- save control; requires destroy/create
        CAnimKnob** temp = new CAnimKnob*[m_nAnimKnobCount + 1];
        for(int i=0; i<m_nAnimKnobCount; i++)
            temp[i] = m_ppKnobControls[i];

        // --- add new one
        temp[m_nAnimKnobCount] = pKnob;
        m_nAnimKnobCount++;

        if(m_ppKnobControls)
            delete m_ppKnobControls;
        m_ppKnobControls = temp;
    }

    return pKnob;
}

void CRafxVSTEditor::initControls(bool bSetListener)
{
	if(!frame)
		return;

	if(!m_pPlugIn)
		return frame->onActivate(true);

	if(!m_pControlMap)
		return frame->onActivate(true);

    if(m_AU && bSetListener)
    {
        // --- create the event listener and tell it the name of our Dispatcher function
        //     EventListenerDispatcher
		verify_noerr(AUEventListenerCreate(EventListenerDispatcher, this,
                                           CFRunLoopGetCurrent(), kCFRunLoopDefaultMode, 0.05, 0.05,
                                           &AUEventListener));
        
        // --- start with first control 0
 		AudioUnitEvent auEvent;
        
        // --- parameter 0
		AudioUnitParameter parameter = {m_AU, 0, kAudioUnitScope_Global, 0};
        
        // --- set param & add it
        auEvent.mArgument.mParameter = parameter;
       	auEvent.mEventType = kAudioUnitEvent_ParameterValueChange;
        verify_noerr(AUEventListenerAddEventType(AUEventListener, this, &auEvent));
        
        // --- notice the way additional params are added using mParameterID
        for(int i=1; i<m_pPlugIn->m_UIControlList.count(); i++)
        {
    		auEvent.mArgument.mParameter.mParameterID = i;
            verify_noerr(AUEventListenerAddEventType(AUEventListener, this, &auEvent));
        }
	}

	for(int i=0; i<m_nXYPadCount; i++)
	{
		if(m_ppXYPads[i]->getTag() == JOYSTICK)
		{
			int nTagX = m_ppXYPads[i]->getTagX();
			int nTagY = m_ppXYPads[i]->getTagY();

			if(nTagX != JOYSTICK_X && nTagY != JOYSTICK_Y)
			{
				float fx = 0;
				float fy = 0;

				// -X
				int nTagX = m_ppXYPads[i]->getTagX();
				if(nTagX != JOYSTICK_X)
				{
					int nControlIndex = m_XMLParser.getTagIndex(nTagX);
					int nPlugInControlIndex = m_pControlMap[nControlIndex];

                    // --- get the normalized value
                    fx = m_pPlugIn->getParameter(nPlugInControlIndex);
				}

				// -Y
				int nTagY = m_ppXYPads[i]->getTagY();
				if(nTagY != JOYSTICK_Y)
				{
					int nControlIndex = m_XMLParser.getTagIndex(nTagY);
					int nPlugInControlIndex = m_pControlMap[nControlIndex];

                    // --- get the normalized value
                    fy = m_pPlugIn->getParameter(nPlugInControlIndex);
				}

				//fy = -1.0*fy + 1.0;
				m_ppXYPads[i]->setValue(m_ppXYPads[i]->calculateValue(fx, fy));
				m_ppXYPads[i]->invalid();
			}
			else
			{
				float fx = (m_fJS_X + 1.0)/2.0;
				float fy = (m_fJS_Y - 1.0)/(-2.0);
				//fy = -1.0*fy + 1.0;

				m_ppXYPads[i]->setValue(m_ppXYPads[i]->calculateValue(fx, fy));
				m_ppXYPads[i]->invalid();
			}
		}
		else if(m_ppXYPads[i]->getTag() == TRACKPAD)// trackpad
		{
			float fx = 0;
			float fy = 0;

			// -X
			int nTagX = m_ppXYPads[i]->getTagX();
			if(nTagX != JOYSTICK_X)
			{
				int nControlIndex = m_XMLParser.getTagIndex(nTagX);
				int nPlugInControlIndex = m_pControlMap[nControlIndex];

                // --- get the normalized value
                fx = m_pPlugIn->getParameter(nPlugInControlIndex);
			}

			// -Y
			int nTagY = m_ppXYPads[i]->getTagY();
			if(nTagY != JOYSTICK_Y)
			{
				int nControlIndex = m_XMLParser.getTagIndex(nTagY);
				int nPlugInControlIndex = m_pControlMap[nControlIndex];

                // --- get the normalized value
                fy = m_pPlugIn->getParameter(nPlugInControlIndex);
			}

			//fy = -1.0*fy + 1.0;
			m_ppXYPads[i]->setValue(m_ppXYPads[i]->calculateValue(fx, fy));
			m_ppXYPads[i]->invalid();
		}
	}

	if(m_ppOptionMenuControls)
	{
		for(int i=0; i<m_nOptionMenuCount; i++)
		{
			COptionMenu* pControl = m_ppOptionMenuControls[i];
			if(pControl)
			{
				if(pControl->getTag() < 0)
					continue;

				// OM's value is 0->string_count-1
				int nControlIndex = m_XMLParser.getTagIndex(pControl->getTag());
				int nPlugInControlIndex = m_pControlMap[nControlIndex];
				CUICtrl* pUICtrl = m_pPlugIn->m_UIControlList.getAt(nPlugInControlIndex);
				if(pUICtrl)
				{
					// --- clear
					pControl->removeAllEntry();

					if(pUICtrl->uUserDataType == UINTData)
					{
						bool bWorking = true;
						int m = 0;
						while(bWorking)
						{
							char* pEnum;
							pEnum = getEnumString(pUICtrl->cEnumeratedList, m);
							if(!pEnum)
								bWorking = false;
							else
							{
								pControl->addEntry(pEnum, m);
								m++;
							}
						}

						// --- set max
						int n = pControl->getNbEntries();
						if(n <= 0)
						{
							pControl->addEntry("-n/a-");
							pControl->setMax(0);
							pControl->setValue(0);
						}

						// --- init
                        pControl->setValue(getInitControlValue(pUICtrl));
					}
					else
					{
						pControl->addEntry("min");
						pControl->addEntry("max");
						pControl->setMax(1);
						pControl->setValue(0);
					}
				}
			}
		}
	}

	if(m_ppVuMeterControls)
	{
		for(int i=0; i<m_nVuMeterCount; i++)
		{
			CVuMeterWP* pControl = m_ppVuMeterControls[i];
			if(pControl)
			{
				if(pControl->getTag() < 0)
					continue;

				int nControlIndex = m_XMLParser.getTagIndex(pControl->getTag());
				int nPlugInControlIndex = m_pControlMap[nControlIndex];
				CUICtrl* pUICtrl = m_pPlugIn->m_UIControlList.getAt(nPlugInControlIndex);
				if(pUICtrl)
				{
					float fSampleRate = 1.0/(METER_UPDATE_INTERVAL_MSEC*0.005	);
					pControl->initDetector(fSampleRate/10.0, pUICtrl->fMeterAttack_ms, pUICtrl->fMeterRelease_ms, true, pUICtrl->uDetectorMode, pUICtrl->bLogMeter);
				}
			}
		}
	}

	// --- radio buttons (Vertical Switches)
	if(m_ppRadioButtonControls)
	{
		for(int i=0; i<m_nRadioButtonCount; i++)
		{
			CVerticalSwitch* pControl = m_ppRadioButtonControls[i]; //->setValue(pControl->getValue());
			if(pControl)
			{
				if(pControl->getTag() < 0)
					continue;

				int32_t nTag = pControl->getTag();
				int nControlIndex = m_XMLParser.getTagIndex(nTag);
				int nPlugInControlIndex = m_pControlMap[nControlIndex];
                pControl->setValueNormalized(m_pPlugIn->getParameter(nPlugInControlIndex));
				pControl->invalid();
			}
		}
	}

	// --- on/off buttons
	if(m_ppOnOffButtonControls)
	{
		for(int i=0; i<m_nOnOffButtonCount; i++)
		{
			COnOffButton* pControl = m_ppOnOffButtonControls[i]; //->setValue(pControl->getValue());
			if(pControl)
			{
				if(pControl->getTag() < 0)
					continue;

				int32_t nTag = pControl->getTag();
				int nControlIndex = m_XMLParser.getTagIndex(nTag);
				int nPlugInControlIndex = m_pControlMap[nControlIndex];
                pControl->setValueNormalized(m_pPlugIn->getParameter(nPlugInControlIndex));
				pControl->invalid();
			}
		}
	}

	// --- sliders
	if(m_ppSliderControls)
	{
		for(int i=0; i<m_nSliderCount; i++)
		{
			CSlider* pControl = m_ppSliderControls[i]; //->setValue(pControl->getValue());
			if(pControl)
			{
				if(pControl->getTag() < 0)
					continue;

				int32_t nTag = pControl->getTag();
				int nControlIndex = m_XMLParser.getTagIndex(nTag);
				int nPlugInControlIndex = m_pControlMap[nControlIndex];
				CUICtrl* pCtrl = m_pPlugIn->m_UIControlList.getAt(nPlugInControlIndex);
                if(pCtrl)
                {
                    if(pCtrl->uUserDataType == UINTData)
                    {
                        float fEnums = (float)getNumEnums(pCtrl->cEnumeratedList);

                        CVerticalSliderWP* vertSlider = dynamic_cast<CVerticalSliderWP*>(pControl);
                        if(vertSlider)
                            vertSlider->setSwitchMax(fEnums - 1.0);

                        CHorizontalSliderWP* horizSlider = dynamic_cast<CHorizontalSliderWP*>(pControl);
                        if(horizSlider)
                            horizSlider->setSwitchMax(fEnums - 1.0);
                    }
                }

                pControl->setValueNormalized(m_pPlugIn->getParameter(nPlugInControlIndex));
				pControl->invalid();
			}
		}
	}

	if(m_ppKnobControls)
	{
		// --- knobs
		for(int i=0; i<m_nAnimKnobCount; i++)
		{
			CAnimKnob* pControl = m_ppKnobControls[i]; //->setValue(pControl->getValue());
			if(pControl)
			{
				if(pControl->getTag() < 0)
					continue;

				int32_t nTag = pControl->getTag();
				int nControlIndex = m_XMLParser.getTagIndex(nTag);
				int nPlugInControlIndex = m_pControlMap[nControlIndex];
                CUICtrl* pCtrl = m_pPlugIn->m_UIControlList.getAt(nPlugInControlIndex);
                if(pCtrl)
                {
                    if(pCtrl->uUserDataType == UINTData)
                    {
                        float fEnums = (float)getNumEnums(pCtrl->cEnumeratedList);
                        ((CKnobWP*)pControl)->setSwitchMax(fEnums - 1.0);
                    }
                }
                pControl->setValueNormalized(m_pPlugIn->getParameter(nPlugInControlIndex));
				pControl->invalid();
			}
		}
	}

	// --- edits
	if(m_ppTextEditControls)
	{
		for(int i=0; i<m_nTextEditCount; i++)
		{
			CTextEdit* pControl = m_ppTextEditControls[i]; //->setValue(pControl->getValue());
			if(pControl)
			{
				if(pControl->getTag() < 0)
					continue;

				int32_t nTag = pControl->getTag();

				// --- do not update the LCD Edit, it is done on its own.
				if(nTag == LCD_KNOB)
					continue;

				int nControlIndex = m_XMLParser.getTagIndex(nTag);
				int nPlugInControlIndex = m_pControlMap[nControlIndex];
                pControl->setValueNormalized(m_pPlugIn->getParameter(nPlugInControlIndex));

				CUICtrl* pCtrl = m_pPlugIn->m_UIControlList.getAt(nPlugInControlIndex);
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
	}

	if(!m_pLCDControlMap || m_nLCDControlCount <= 0)
		return frame->onActivate(true);

	// --- is there an alpha control?
	if(!getAlphaWheelKnob() && !getLCDValueKnob())
		return frame->onActivate(true);

	// --- update AW
	getAlphaWheelKnob()->setValue(calcSliderVariable(0, m_nLCDControlCount-1, m_nAlphaWheelIndex));

	// --- update everything else
	updateLCDGuts(m_nAlphaWheelIndex);

	frame->onActivate(true);
}
    
float CRafxVSTEditor::getCookedValue(int nIndex, float fNormalizedValue)
{
	CUICtrl* pUICtrl = m_pPlugIn->m_UIControlList.getAt(nIndex);
	if(!pUICtrl)
	{
		return 0.0;
	}
    
    float fCookedValue = 0.0;
    
	// auto cook the data first
	switch(pUICtrl->uUserDataType)
	{
		case intData:
			fCookedValue = calcDisplayVariable(pUICtrl->fUserDisplayDataLoLimit, pUICtrl->fUserDisplayDataHiLimit, fNormalizedValue);
			break;
            
		case floatData:
			fCookedValue = calcDisplayVariable(pUICtrl->fUserDisplayDataLoLimit, pUICtrl->fUserDisplayDataHiLimit, fNormalizedValue);
			break;
            
		case doubleData:
			fCookedValue = calcDisplayVariable(pUICtrl->fUserDisplayDataLoLimit, pUICtrl->fUserDisplayDataHiLimit, fNormalizedValue);
			break;
            
		case UINTData:
			fCookedValue = calcDisplayVariable(pUICtrl->fUserDisplayDataLoLimit, pUICtrl->fUserDisplayDataHiLimit, fNormalizedValue);
			break;
            
		default:
			break;
	}
    
    return fCookedValue;
}

//-----------------------------------------------------------------------------------
void CRafxVSTEditor::valueChanged(CControl* pControl)
{
	if(!m_pPlugIn) return;
	if(!m_pControlMap) return;

	CUICtrl* pCtrl = NULL;

	int32_t nTag = pControl->getTag();
	if(nTag == ASSIGNBUTTON_1)
	{
		m_pPlugIn->userInterfaceChange(50);
		return;
	}
	else if(nTag == ASSIGNBUTTON_2)
	{
		m_pPlugIn->userInterfaceChange(51);
		return;
	}
	else if(nTag == ASSIGNBUTTON_3)
	{
		m_pPlugIn->userInterfaceChange(52);
		return;
	}
	else if(nTag == JOYSTICK)
	{
		// --- find this control
		CXYPadWP* pPad = findJoystickXYPad(pControl);

		if(pPad)
		{
			if(pPad->getTagX() == JOYSTICK_X && pPad->getTagY() == JOYSTICK_Y)
			{
				float x, y = 0;

				pPad->calculateXY(pPad->getValue(), x, y);
				m_fJS_X = 2.0*x - 1.0;
				m_fJS_Y = -2.0*y + 1.0;

				double dA, dB, dC, dD, dAC, dBD;
				calculateVectorMixValues(0.0, 0.0, m_fJS_X, m_fJS_Y, dA, dB, dC, dD, dAC, dBD, 1, true);
				m_pPlugIn->joystickControlChange(dA, dB, dC, dD, dAC, dBD);
				return;
			}
			else
			{
				int nControlIndex = m_XMLParser.getTagIndex(pPad->getTagX());
				int nPlugInControlIndex = m_pControlMap[nControlIndex];
				float x, y = 0;

				pPad->calculateXY(pPad->getValue(), x, y);
				y = -1.0*y + 1.0;

				// --- get the control for re-broadcast of some types
				pCtrl = m_pPlugIn->m_UIControlList.getAt(nPlugInControlIndex);

				float fPluginValue = getPluginParameterValue(pCtrl, x, NULL);
                
                // --- make an AudioUnitParameter set in our AU buddy
                AudioUnitParameter paramX = {m_AU, nPlugInControlIndex, kAudioUnitScope_Global, 0};
                
                // --- set the AU Parameter; this calls SetParameter() in the au
                AUParameterSet(AUEventListener, (void*)this, &paramX, getCookedValue(nPlugInControlIndex, fPluginValue), 0);
                
				// --- now broadcast to all controls with same tag value
				broadcastControlChange(pPad->getTagX(), fPluginValue, x, pControl, pCtrl);

				nControlIndex = m_XMLParser.getTagIndex(pPad->getTagY());
				nPlugInControlIndex = m_pControlMap[nControlIndex];

				// --- get the control for re-broadcast of some types
				pCtrl = m_pPlugIn->m_UIControlList.getAt(nPlugInControlIndex);

				fPluginValue = getPluginParameterValue(pCtrl, y, NULL);

                // --- make an AudioUnitParameter set in our AU buddy
                AudioUnitParameter paramY = {m_AU, nPlugInControlIndex, kAudioUnitScope_Global, 0};
                
                // --- set the AU Parameter; this calls SetParameter() in the au
                AUParameterSet(AUEventListener, (void*)this, &paramY, getCookedValue(nPlugInControlIndex, fPluginValue), 0);

				// --- now broadcast to all controls with same tag value
				broadcastControlChange(pPad->getTagY(), fPluginValue, y, pControl, pCtrl);
				return;
			}
		}
	}
	else if(nTag == TRACKPAD)
	{
		// --- find this control
		CXYPadWP* pPad = findJoystickXYPad(pControl);

		if(pPad)
		{
			int nControlIndex = m_XMLParser.getTagIndex(pPad->getTagX());
			int nPlugInControlIndex = m_pControlMap[nControlIndex];
			float x, y = 0;

			pPad->calculateXY(pPad->getValue(), x, y);
			y = -1.0*y + 1.0;

			// --- get the control for re-broadcast of some types
            pCtrl = m_pPlugIn->m_UIControlList.getAt(nPlugInControlIndex);

			float fPluginValue = getPluginParameterValue(pCtrl, x, NULL);

            // --- make an AudioUnitParameter set in our AU buddy
            AudioUnitParameter paramX = {m_AU, nPlugInControlIndex, kAudioUnitScope_Global, 0};
            
            // --- set the AU Parameter; this calls SetParameter() in the au
            AUParameterSet(AUEventListener, this, &paramX, getCookedValue(nPlugInControlIndex, fPluginValue), 0);

            // --- now broadcast to all controls with same tag value
            broadcastControlChange(pPad->getTagX(), fPluginValue, x, pControl, pCtrl);

			nControlIndex = m_XMLParser.getTagIndex(pPad->getTagY());
			nPlugInControlIndex = m_pControlMap[nControlIndex];

            // --- get the control for re-broadcast of some types
            pCtrl = m_pPlugIn->m_UIControlList.getAt(nPlugInControlIndex);

			fPluginValue = getPluginParameterValue(pCtrl, y, NULL);
         
            // --- make an AudioUnitParameter set in our AU buddy
            AudioUnitParameter paramY = {m_AU, nPlugInControlIndex, kAudioUnitScope_Global, 0};

            // --- set the AU Parameter; this calls SetParameter() in the au
            AUParameterSet(AUEventListener, (void*)this, &paramY, getCookedValue(nPlugInControlIndex, fPluginValue), 0);

            // --- now broadcast to all controls with same tag value
            broadcastControlChange(pPad->getTagY(), fPluginValue, y, pControl, pCtrl);
			return;
		}
	}
	else if(nTag == ALPHA_WHEEL)
	{
		int nAWI = (int)(calcDisplayVariable(0, m_nLCDControlCount-1, pControl->getValue()));

		if(nAWI != m_nAlphaWheelIndex)
		{
			m_nAlphaWheelIndex = nAWI;

			// --- this gets the new value to broadcast
			float fValue = updateLCDGuts(m_nAlphaWheelIndex);

			// --- now broadcast to all controls with same tag value
			CUICtrl* pCtrl = m_pPlugIn->m_UIControlList.getAt(m_pLCDControlMap[m_nAlphaWheelIndex]);
			if(pCtrl)
				broadcastControlChange(m_pLCDControlMap[m_nAlphaWheelIndex], fValue, fValue, pControl, pCtrl);
		}
		return;
	}
	else if(nTag == LCD_KNOB)
	{
		// --- get the control
		pCtrl = m_pPlugIn->m_UIControlList.getAt(m_pLCDControlMap[m_nAlphaWheelIndex]);
		if(!pCtrl) return;

		// --- set the parameter info
		float fValue = getPluginParameterValue(pCtrl, pControl->getValue(), NULL);
     
        // --- make an AudioUnitParameter set in our AU buddy
        AudioUnitParameter param = {m_AU, m_pLCDControlMap[m_nAlphaWheelIndex], kAudioUnitScope_Global, 0};
        
        // --- set the AU Parameter; this calls SetParameter() in the au
        AUParameterSet(AUEventListener, (void*)this, &param, getCookedValue(m_pLCDControlMap[m_nAlphaWheelIndex], fValue), 0);

		if(isKnobControl(pControl))
		{
			// --- update the edit control
			updateTextEdits(LCD_KNOB, pControl, pCtrl);
		}
		else if(isTextEditControl(pControl))
		{
			// --- self update, calc string index if needed, returns 0->1 value to update other controls
			float fValue = updateEditControl(pControl, pCtrl);

			// --- update the LCDKnob
			updateAnimKnobs(LCD_KNOB, fValue);
		}

		// --- now broadcast to all controls with same tag value
		broadcastControlChange(m_pLCDControlMap[m_nAlphaWheelIndex], fValue, pControl->getValue(), pControl, pCtrl);
		return;
	}

	int nControlIndex = m_XMLParser.getTagIndex(nTag);
	int nPlugInControlIndex = m_pControlMap[nControlIndex];

	// --- get the control for re-broadcast of some types
	pCtrl = m_pPlugIn->m_UIControlList.getAt(nPlugInControlIndex);

	float fControlValue = 0.0;
	// --- check text edits
	if(isTextEditControl(pControl))
	{
		// --- self update, calc string index if needed, returns 0->1 value to update other controls
		fControlValue = updateEditControl(pControl, pCtrl);
	}
	else
		fControlValue = pControl->getValue();

	// --- first set the param on the plugin
	float fPluginValue = getPluginParameterValue(pCtrl, fControlValue, pControl);
   
    // --- make an AudioUnitParameter set in our AU buddy
    AudioUnitParameter param = {m_AU, nPlugInControlIndex, kAudioUnitScope_Global, 0};
    
    // --- set the AU Parameter; this calls SetParameter() in the au
    AUParameterSet(AUEventListener, (void*)this, &param, getCookedValue(nPlugInControlIndex, fPluginValue), 0);
 
	// --- now broadcast to all controls with same tag value
	broadcastControlChange(nTag, fPluginValue, fControlValue, pControl, pCtrl);
}

const char* CRafxVSTEditor::getEnumString(const char* sTag)
{
    if(!m_pPlugIn) return NULL;

    CUICtrl* pCtrl;
    int nControlIndex = m_XMLParser.getTagIndex(sTag);
    int nPlugInControlIndex = m_pControlMap[nControlIndex];

    pCtrl = m_pPlugIn->m_UIControlList.getAt(nPlugInControlIndex);
    if(pCtrl)
    {
        const char* cEnum(pCtrl->cEnumeratedList);
        return cEnum;
    }

    return NULL;
}

const char* CRafxVSTEditor::getBitmapName(int nIndex)
{
    return m_XMLParser.getBitmapName(nIndex);
}

const char* CRafxVSTEditor::getRAFXBitmapType(const char* bitmapName)
{
    return m_XMLParser.getRAFXBitmapType(bitmapName);
}

const char* CRafxVSTEditor::getCurrentBackBitmapName()
{
    const char_t* bitmap = m_XMLParser.getTemplateAttribute("Editor", "bitmap");
    if(strlen(bitmap) > 0)
        return bitmap;

    return NULL;
}

const char* CRafxVSTEditor::getCurrentBackColorName()
{
    const char_t* bitmap = m_XMLParser.getTemplateAttribute("Editor", "background-color");
    if(strlen(bitmap) > 0)
        return bitmap;

    return NULL;
}



}