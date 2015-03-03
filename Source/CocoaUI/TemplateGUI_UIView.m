/*	Copyright  2013 Tritone Systems, Inc. All Rights Reserved.
 
 Disclaimer: IMPORTANT:  This software is supplied to you by 
 Tritone Systems Inc. ("Tritone Systems") in consideration of your agreement to the
 following terms, and your use, installation, modification or
 redistribution of this Tritone Systems software constitutes acceptance of these
 terms.  If you do not agree with these terms, please do not use,
 install, modify or redistribute this Tritone Systems software.
 
 The Tritone Systems Software is provided by Tritone Systems on an "AS IS" basis.  
 Tritone Systems:
 MAKES NO WARRANTIES, EXPRESS OR IMPLIED, INCLUDING WITHOUT LIMITATION
 THE IMPLIED WARRANTIES OF NON-INFRINGEMENT, MERCHANTABILITY AND FITNESS
 FOR A PARTICULAR PURPOSE, REGARDING THE Tritone Systems SOFTWARE OR ITS USE AND
 OPERATION ALONE OR IN COMBINATION WITH YOUR PRODUCTS.
 
 IN NO EVENT SHALL Tritone Systems BE LIABLE FOR ANY SPECIAL, INDIRECT, INCIDENTAL
 OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 INTERRUPTION) ARISING IN ANY WAY OUT OF THE USE, REPRODUCTION,
 MODIFICATION AND/OR DISTRIBUTION OF THE Tritone Systems SOFTWARE, HOWEVER CAUSED
 AND WHETHER UNDER THEORY OF CONTRACT, TORT (INCLUDING NEGLIGENCE),
 STRICT LIABILITY OR OTHERWISE, EVEN IF Tritone Systems HAS BEEN ADVISED OF THE
 POSSIBILITY OF SUCH DAMAGE.
 
 This software is based on the free sample code provided by Apple called FilterDemo. I modified 
 this demo software. Please see the original source at:
 
 https://developer.apple.com/library/mac/#samplecode/FilterDemo/Introduction/Intro.html#//apple_ref/doc/uid/DTS10003570
 
 for more information.
 */

#import "TemplateGUI_UIView.h"
#import "RafxVSTEditor.h" // only needed for VIEW_STRUCT


enum
{
	// index values to link your controls to the GUI (from RackAFX)
	km_uLFO1WaveformIndex,
	km_dLFO1RateIndex,
	km_uLFO2WaveformIndex,
	km_dLFO2RateIndex,
	km_dOp1RatioIndex,
	km_dEG1Attack_mSecIndex,
	km_dEG1Decay_mSecIndex,
	km_dEG1SustainLevelIndex,
	km_dEG1Release_mSecIndex,
	km_dOp1OutputLevelIndex,
	km_dFilterEGAttack_mSecIndex,
	km_dFilterEGDecay_mSecIndex,
	km_dFilterEGSustainLevelIndex,
	km_dFilterEGRelease_mSecIndex,
	km_dOp2RatioIndex,
	km_dEG2Attack_mSecIndex,
	km_dEG2Decay_mSecIndex,
	km_dEG2SustainLevelIndex,
	km_dEG2Release_mSecIndex,
	km_dOp2OutputLevelIndex,
	km_dNoiseGenEGAttack_mSecIndex,
	km_dNoiseGenEGDecay_mSecIndex,
	km_dNoiseGenEGSustainLevelIndex,
	km_dNoiseGenEGRelease_mSecIndex,
	km_dOp3RatioIndex,
	km_dEG3Attack_mSecIndex,
	km_dEG3Decay_mSecIndex,
	km_dEG3SustainLevelIndex,
	km_dEG3Release_mSecIndex,
	km_dOp3OutputLevelIndex,
	km_dFilterFcControlIndex,
	km_dFilterQControlIndex,
	km_dNoiseGenOutputLevelIndex,
	km_dOp4FeedbackIndex,
	km_dOp4RatioIndex,
	km_dEG4Attack_mSecIndex,
	km_dEG4Decay_mSecIndex,
	km_dEG4SustainLevelIndex,
	km_dEG4Release_mSecIndex,
	km_dOp4OutputLevelIndex,
	km_uVoiceModeIndex,
	km_dVolume_dBIndex,
	km_dPortamentoTime_mSecIndex,
	km_uLegatoModeIndex,
	km_uResetToZeroIndex,
	km_nPitchBendRangeIndex,
	km_dLFO1IntensityIndex,
	km_dLFO2IntensityIndex,
	km_uLFO1ModDest1Index,
	km_uLFO1ModDest2Index,
	km_uLFO1ModDest3Index,
	km_uLFO1ModDest4Index,
	km_uLFO2ModDest1Index,
	km_uLFO2ModDest2Index,
	km_uLFO2ModDest3Index,
	km_uLFO2ModDest4Index,
	km_uNoiseGenWaveformIndex,
	km_uLFO1ModDestNoiseAmpIndex,
	km_uLFO2ModDestNoiseAmpIndex,
	km_dLFO1PanIntensityIndex,
	km_dLFO2PanIntensityIndex,
	km_dLFO1AmpIntensityIndex,
	km_dLFO2AmpIntensityIndex,
	km_dLFO1FCutIntensityIndex,
	km_dLFO2FCutIntensityIndex,
	km_uFilterTypeControlIndex,
	km_dFilterEGModIntensityControlIndex,
	km_dPanControlIndex,
	km_uOp1WaveformIndex,
	km_uOp2WaveformIndex,
	km_uOp3WaveformIndex,
	km_uOp4WaveformIndex,
	kNumberOfParameters
};



#pragma mark ____ LISTENER CALLBACK DISPATCHER ____

@implementation TemplateGUI_UIView

enum
{
	kOpenGUI = 64000,
    kCloseGUI
};

- (id) initWithVSTGUI:(AudioUnit)AU preferredSize:(NSSize)size
{
   	self = [super initWithFrame:NSMakeRect (0, 0, size.width, size.height)];
  
  	if(self)
        mAU = AU;
    
    return self;
}

// --- use normal drawing coords
- (BOOL)isFlipped { return YES; }

// --- change size of frame
- (void) setFrame:(NSRect)newSize
{
	[super setFrame: newSize];
}

- (void)willRemoveSubview:(NSView *)subview
{
    VIEW_STRUCT viewStruct;
    viewStruct.pWindow =(void*)self;
    UInt32 size = sizeof(viewStruct);

    // --- open VSTGUI editor
	if (AudioUnitSetProperty (mAU, kCloseGUI, kAudioUnitScope_Global, 0, (void*)&viewStruct, size) != noErr)
		return;
}


/*
- (BOOL) isOpaque {
	return YES;
}*/

@end

