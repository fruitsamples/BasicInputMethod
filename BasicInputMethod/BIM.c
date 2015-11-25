/*    File:	 BIM.c    Description: Core implementation of our text service.    Author:	 SC    Copyright: 	 � Copyright 2000-2001 Apple Computer, Inc. All rights reserved.    Disclaimer:	 IMPORTANT:  This Apple software is supplied to you by Apple Computer, Inc.                 ("Apple") in consideration of your agreement to the following terms, and your                 use, installation, modification or redistribution of this Apple software                 constitutes acceptance of these terms.  If you do not agree with these terms,                 please do not use, install, modify or redistribute this Apple software.                 In consideration of your agreement to abide by the following terms, and subject                 to these terms, Apple grants you a personal, non-exclusive license, under Apple�s                 copyrights in this original Apple software (the "Apple Software"), to use,                 reproduce, modify and redistribute the Apple Software, with or without                 modifications, in source and/or binary forms; provided that if you redistribute                 the Apple Software in its entirety and without modifications, you must retain                 this notice and the following text and disclaimers in all such redistributions of                 the Apple Software.  Neither the name, trademarks, service marks or logos of                 Apple Computer, Inc. may be used to endorse or promote products derived from the                 Apple Software without specific prior written permission from Apple.  Except as                 expressly stated in this notice, no other rights or licenses, express or implied,                 are granted by Apple herein, including but not limited to any patent rights that                 may be infringed by your derivative works or by other works in which the Apple                 Software may be incorporated.                 The Apple Software is provided by Apple on an "AS IS" basis.  APPLE MAKES NO                 WARRANTIES, EXPRESS OR IMPLIED, INCLUDING WITHOUT LIMITATION THE IMPLIED                 WARRANTIES OF NON-INFRINGEMENT, MERCHANTABILITY AND FITNESS FOR A PARTICULAR                 PURPOSE, REGARDING THE APPLE SOFTWARE OR ITS USE AND OPERATION ALONE OR IN                 COMBINATION WITH YOUR PRODUCTS.                 IN NO EVENT SHALL APPLE BE LIABLE FOR ANY SPECIAL, INDIRECT, INCIDENTAL OR                 CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE                 GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)                 ARISING IN ANY WAY OUT OF THE USE, REPRODUCTION, MODIFICATION AND/OR DISTRIBUTION                 OF THE APPLE SOFTWARE, HOWEVER CAUSED AND WHETHER UNDER THEORY OF CONTRACT, TORT                 (INCLUDING NEGLIGENCE), STRICT LIABILITY OR OTHERWISE, EVEN IF APPLE HAS BEEN                 ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.    Change History (most recent first):                 2001/03/28	SC	Removed code that disabled the Show/Hide Debug Palette menu                                        item.                 2001/03/09	SC	Added a change to the menu attributes so that a pencil glyph                                        will be substituted for the standard control key glyph in                                        the pencil menu (this is for JIS keyboards that feature a                                        pencil glyph on the control key).                                        Implemented the Show/Hide Debug Palette menu item.                 2000/07/28	SC	Changed to include Carbon.h                 2000/06/01	SC	Created*/#define TARGET_API_MAC_CARBON 1#include <Carbon/Carbon.h>#include "BIM.h"#include "BIMInputEvents.h"#include "BIMLaunchServer.h"#include "BIMMessageReceive.h"#include "BIMMessageSend.h"#include "BIMClientServer.h"#include "BIMScript.h"//  Constantsconst short kMENU_Pencil = kBaseResourceID + 1;const short kICNx_Pencil = kBaseResourceID + 1;enum {    kShowHideKeyboardPaletteMenuItem = 1,		//  Menu item indices.    kShowHideSendEventPaletteMenuItem = 2,    kConvertToLowercaseMenuItem = 4,    kConvertToUppercaseMenuItem = 5};enum {    kShowHideKeyboardPaletteMenuCommand = 'SHKP',	//  Command IDs for our pencil menu items.    kShowHideSendEventPaletteMenuCommand = 'SHDP',    kConvertToLowercaseMenuCommand = 'CLOW',    kConvertToUppercaseMenuCommand = 'CUPP'};const short kSTRx_MenuItems = kBaseResourceID + 1;enum {    kShowKeyboardPaletteMenuItemString = 1,		//  String indices for menu item text.    kHideKeyboardPaletteMenuItemString = 2,    kShowSendEventPaletteMenuItemString = 3,    kHideSendEventPaletteMenuItemString = 4};//  Global variablesstatic BIMSessionHandle gActiveSession;static MenuRef gPencilMenu;static CFStringRef gShowKeyboardPaletteMenuItemString;static CFStringRef gHideKeyboardPaletteMenuItemString;static CFStringRef gShowSendEventPaletteMenuItemString;static CFStringRef gHideSendEventPaletteMenuItemString;//  Local functionsstatic OSStatus BIMConvertToLowercase( BIMSessionHandle inSessionHandle );static OSStatus BIMConvertToUppercase( BIMSessionHandle inSessionHandle );static pascal OSStatus BIMPencilMenuEventHandler( EventHandlerCallRef inEventHandlerCallRef,                                                  EventRef inEventRef, void *inUserData );/**************************************************************************************************  BIMInitialize**  This routine is called the first time our text service component is instantiated. It is*  called via NewTSMDocument.**  In this routine we initialize our global state (initialize global variables, launch the*  server process, and set up the text service pencil menu). We only initialize global data*  here. All per-session context initialization is handled in BIMSessionOpen().**	In	inComponentInstance	The component instance.**	Out	outTextServiceMenu	Our text service menu ref.*		ComponentResult		A toolbox error code.*************************************************************************************************/ComponentResult BIMInitialize( ComponentInstance inComponentInstance,                               MenuRef *outTextServiceMenu ){    ComponentResult	result;    short		refNum;    EventTypeSpec	menuEventSpec;    Handle		iconData;    Handle		menuIconSuite;    Str255		menuText;        result = noErr;    refNum = -1;        //  Initialize our global variables.        gActiveSession = NULL;    gPencilMenu = NULL;        //  Initialize the server, launching it if necessary.        result = BIMLaunchServer();    //	Open our component's resource fork.    if( result == noErr ) {        refNum = OpenComponentResFile( (Component) inComponentInstance );        result = ResError();        if( ( result == noErr ) && ( refNum == -1 ) )            result = resFNotFound;    }        //	Load the pencil menu.        if( result == noErr ) {        gPencilMenu = GetMenu( kMENU_Pencil );        if( gPencilMenu )            *outTextServiceMenu = gPencilMenu;        else            result = resNotFound;    }    //  Indicate that a pencil glyph should be substituted for the standard control key glyph    //  (for JIS keyboards which have a pencil icon on the control key).        if( result == noErr )        ChangeMenuAttributes( gPencilMenu, kMenuAttrUsePencilGlyph, 0 );        //	Install an event handler to receive menu commands.    if( result == noErr ) {        menuEventSpec.eventClass = kEventClassCommand;        menuEventSpec.eventKind = kEventProcessCommand;        result = InstallMenuEventHandler( gPencilMenu,                                          NewEventHandlerUPP( BIMPencilMenuEventHandler ), 1,                                          &menuEventSpec, nil, nil );    }    //	Create the pencil icon suite, then load and detach the resources.    if( result == noErr )        result = NewIconSuite( &menuIconSuite );    if( result == noErr ) {        iconData = GetResource( 'ics8', kICNx_Pencil );        if( iconData == nil )            result = resNotFound;        else            DetachResource( iconData );    }    if( result == noErr )        result = AddIconToSuite( iconData, menuIconSuite, 'ics8' );    if( result == noErr ) {        iconData = GetResource( 'ics4', kICNx_Pencil );        if( iconData == nil )            result = resNotFound;        else            DetachResource( iconData );    }    if( result == noErr )        result = AddIconToSuite( iconData, menuIconSuite, 'ics4' );    if( result == noErr ) {        iconData = GetResource( 'ics#', kICNx_Pencil );        if( iconData == nil )            result = resNotFound;        else            DetachResource( iconData );    }    if( result == noErr )        result = AddIconToSuite( iconData, menuIconSuite, 'ics#' );    //	Set the menu title to the pencil icon.    if( result == noErr ) {        menuText[0] = 5;        menuText[1] = 1;        *(Handle *)( &menuText[2] ) = menuIconSuite;        SetMenuTitle( gPencilMenu, menuText );    }        //	Load the menu item strings.        if( result == noErr ) {        GetIndString( menuText, kSTRx_MenuItems, kShowKeyboardPaletteMenuItemString );        gShowKeyboardPaletteMenuItemString = CFStringCreateWithPascalString( NULL, menuText,                                                                             kTextEncodingMacRoman );        GetIndString( menuText, kSTRx_MenuItems, kHideKeyboardPaletteMenuItemString );        gHideKeyboardPaletteMenuItemString = CFStringCreateWithPascalString( NULL, menuText,                                                                             kTextEncodingMacRoman );        GetIndString( menuText, kSTRx_MenuItems, kShowSendEventPaletteMenuItemString );        gShowSendEventPaletteMenuItemString = CFStringCreateWithPascalString( NULL, menuText,                                                                              kTextEncodingMacRoman );        GetIndString( menuText, kSTRx_MenuItems, kHideSendEventPaletteMenuItemString );        gHideSendEventPaletteMenuItemString = CFStringCreateWithPascalString( NULL, menuText,                                                                              kTextEncodingMacRoman );    }    //	Close our component's resource fork.    if( refNum != -1 )        CloseComponentResFile( refNum );        //  Initialize everything needed to receive messages.        if( result == noErr )        result = BIMInitializeMessageReceiving();    //	Return to caller.    return result;}/**************************************************************************************************  BIMTerminate**  This routine is called when the last instance of our component is terminated. It is called*  via DeleteTSMDocument.**  In this routine we dispose of our global state. We do not handle per-session stuff in this*  routine.**	In	inComponentInstance	The component instance.*************************************************************************************************/void BIMTerminate( ComponentInstance inComponentInstance ){    //  Clear our global variables.        gActiveSession = NULL;    gPencilMenu = NULL;}#pragma mark -/**************************************************************************************************  BIMSessionOpen**  This routine is called via NewTSMDocument.**  In this routine, we initialize a new session context. We create a session handle to store all*  of the information pertinent to the session and initialize all its data structures. We do not*  handle initialize global data (data that is shared across sessions); that is taken care of by*  BIMInitialize().**	In	inComponentInstance	The component instance.**	Out	outSessionHandle	The new session context.*		ComponentResult		A toolbox error code.*************************************************************************************************/ComponentResult BIMSessionOpen( ComponentInstance inComponentInstance,                                BIMSessionHandle *outSessionHandle ){    ComponentResult result;    result = noErr;        //	If per-session storage is not set up yet, do so now.    if( result == noErr ) {        if( *outSessionHandle == nil )            *outSessionHandle =( BIMSessionHandle ) NewHandle( sizeof( BIMSessionRecord ) );        if( *outSessionHandle ) {                //	Initialize the contents.                ( **outSessionHandle )->fComponentInstance = inComponentInstance;            ( **outSessionHandle )->fLastUpdateLength = 0;            ( **outSessionHandle )->fInputBufferCount = 0;            ( **outSessionHandle )->fInputBuffer = nil;        }        else            result = memFullErr;    }            //	Initialize the input buffer.    if( result == noErr ) {        ( **outSessionHandle )->fInputBuffer = (UniCharPtr) NewPtr( 1024 );        result = MemError();    }    //	Return result.    return result;}/**************************************************************************************************  BIMSessionClose**  This routine is called via DeleteTSMDocument.**  In this routine, we destroy the current session context. This includes all per-session data*  that is stored in the session handle and the session handle itself.**	In	inSessionHandle		The session context to destroy.*************************************************************************************************/void BIMSessionClose( BIMSessionHandle inSessionHandle ){    if( inSessionHandle ) {            //  Dispose of our input buffer.            if( ( *inSessionHandle )->fInputBuffer )            DisposePtr( (Ptr) ( *inSessionHandle )->fInputBuffer );                    //  Dispose if the session handle itself.                    DisposeHandle( (Handle) inSessionHandle );    }}/**************************************************************************************************  BIMSessionActivate**  This routine is called by the Text Services Manager whenever an application calls*  NewTSMDocument or ActivateTSMDocument. The appropriate response to ActivateTextService is to*  restore our active state, including displaying all floating windows if they have been hidden,*  and redisplaying any inconfirmed text in the currently active input area.**	In	inSessionHandle		The session context.**	Out	ComponentResult		A toolbox error code.*************************************************************************************************/ComponentResult BIMSessionActivate( BIMSessionHandle inSessionHandle ){    OSStatus result;    Boolean isHidden;    gActiveSession = inSessionHandle;        //  When we are activated, we register ourselves with the server process.    result = BIMSendActivatedMessage();    //  Update the menu items.        if( result == noErr )        result = BIMIsKeyboardPaletteHidden( &isHidden );    if( result == noErr )        BIMUpdateShowHideKeyboardPaletteMenuItem( isHidden );    if( result == noErr )        result = BIMIsSendEventPaletteHidden( &isHidden );    if( result == noErr )        BIMUpdateShowHideSendEventPaletteMenuItem( isHidden );    return result;}/**************************************************************************************************  BIMSessionDeactivate**  This routine is called by the Text Services Manager whenever an application calls*  DeactivateTSMDocument. We are responsible for saving whatever state information we need to*  save so that we can restore it again when we are reactivated. We should not confirm any*  unconfirmed text in the active input area, but save it until reactivation. We should not hide*  our floating windows either.**	In	inSessionHandle		The session context.**	Out	ComponentResult		A toolbox error code.*************************************************************************************************/ComponentResult BIMSessionDeactivate( BIMSessionHandle inSessionHandle ){    gActiveSession = nil;    //  We have been deactivated, so deregister ourselves with the server process.    return BIMSendDeactivatedMessage();}/**************************************************************************************************  BIMSessionEvent**  This routine is called by the Text Services Manager in response to mouse and keyboard events.*  The events are passed in via eventRef. We use the standard Carbon Event Manager calls*  (GetEventClass, GetEventKind, and GetEventParameter) to extract information from the event.**	In	inSessionHandle		The session context.*		inEventRef		An eventRef describing the user event.**	Out	ComponentResult		True if we handled the event, otherwise false.*************************************************************************************************/ComponentResult BIMSessionEvent( BIMSessionHandle inSessionHandle, EventRef inEventRef ){    Boolean handled;    UInt32 eventClass;    UInt32 eventKind;    handled = FALSE;    //  Extract the event class and kind.    eventClass = GetEventClass( inEventRef );    eventKind = GetEventKind( inEventRef );    //  For now, we're only interested in the 2 raw keyboard events kEventRawKeyDown and    //  kEventRawKeyRepeat.    //    //  kEventClassKeyboard:    //	  kEventRawKeyDown - A key was pressed    //	  kEventRawKeyRepeat - Sent periodically as a key is held down by the user    //	  kEventRawKeyUp - A key was released    //	  kEventRawKeyModifiersChanged - The keyboard modifiers (bucky bits) have changed    if( eventClass == kEventClassKeyboard && ( eventKind == kEventRawKeyDown ||                                               eventKind == kEventRawKeyRepeat ) ) {        UInt32 keyCode;        unsigned char charCode;        UInt32 modifiers;                //  Extract the key code parameter (kEventParamKeyCode).                GetEventParameter( inEventRef, kEventParamKeyCode, typeUInt32, nil, sizeof (keyCode),                           nil, &keyCode );                //  Extract the character code parameter (kEventParamKeyMacCharCodes).                GetEventParameter( inEventRef, kEventParamKeyMacCharCodes, typeChar, nil,                           sizeof( charCode ), nil, &charCode );                //  Extract the modifiers parameter (kEventParamKeyModifiers).                GetEventParameter( inEventRef, kEventParamKeyModifiers, typeUInt32, nil,                           sizeof( modifiers ), nil, &modifiers );        //  Handle the key.                handled = BIMHandleInput( inSessionHandle, charCode );    }    return handled;}/**************************************************************************************************  BIMSessionFix**  This routine is called via FixTSMDocument.**  In this routine we fix all the active text (send any remaining text in the currently active*  inline area to the application).**	In	inSessionHandle		The session context.**	Out	ComponentResult		A toolbox error code.*************************************************************************************************/ComponentResult BIMSessionFix( BIMSessionHandle inSessionHandle ){    ComponentResult result;    if( ( *inSessionHandle )->fInputBufferCount ) {        result = BIMUpdateActiveInputArea( inSessionHandle, true );        ( *inSessionHandle )->fLastUpdateLength = 0;        ( *inSessionHandle )->fInputBufferCount = 0;    }    return result;}/**************************************************************************************************  BIMSessionHidePalettes**  Handles requests from the Text Services Manager or the applicatin to hide our palette windows.*  The palettes are managed by the server, so here we send a message to the server process,*  telling it to hide any open palette windows.**	In	inSessionHandle		The current session.**	Out	ComponentResult		A toolbox error code.*************************************************************************************************/ComponentResult BIMSessionHidePalettes( BIMSessionHandle inSessionHandle ){    return BIMSendHidePalettesMessage();}#pragma mark -/**************************************************************************************************  BIMGetActiveSession**  Returns the current active session, or NULL if there is no active session.**	Out	BIMSessionHandle		The current active session.*************************************************************************************************/BIMSessionHandle BIMGetActiveSession( void ){    return gActiveSession;}/**************************************************************************************************  BIMHandleInput**  Handles input from the keyboard or via a message from the server process.**	In	inSessionHandle		The current session.*		inCharCode		The character code of the keypress.**	Out	Boolean			TRUE if the input was handled, FALSE otherwise.*************************************************************************************************/Boolean BIMHandleInput( BIMSessionHandle inSessionHandle, unsigned char inCharCode ){    Boolean handled;        handled = FALSE;        switch( inCharCode ) {        //  Handle the delete key. Here we do the most simple processing -- removing the last        //  character from the input buffer.        case 0x08:            if( ( *inSessionHandle )->fInputBufferCount ) {                (( *inSessionHandle )->fInputBufferCount)--;                BIMUpdateActiveInputArea( inSessionHandle, FALSE );                handled = TRUE;            }            break;        //  Handle the enter and return keys by fixing the current inline session.        case 0x03:        case 0x0D:            if( ( *inSessionHandle )->fInputBufferCount ) {                BIMSessionFix( inSessionHandle );                handled = TRUE;            }            break;                    //  Handle all other keys.                    default:            ( *inSessionHandle )->fInputBuffer [ ( *inSessionHandle )->fInputBufferCount ] = inCharCode;            (( *inSessionHandle )->fInputBufferCount)++;            BIMUpdateActiveInputArea( inSessionHandle, FALSE );            handled = TRUE;            break;    }    return handled;}/**************************************************************************************************  BIMUpdateShowHideKeyboardPaletteMenuItem**  Updates the "Show Keyboard Palette"/"Hide Keyboard Palette" menu item text as appropriate.**	In	inIsHidden		The current visible status of the keyboard palette.*************************************************************************************************/void BIMUpdateShowHideKeyboardPaletteMenuItem( Boolean inIsHidden ){    if( inIsHidden )        SetMenuItemTextWithCFString( gPencilMenu, kShowHideKeyboardPaletteMenuItem,                                     gShowKeyboardPaletteMenuItemString );    else        SetMenuItemTextWithCFString( gPencilMenu, kShowHideKeyboardPaletteMenuItem,                                     gHideKeyboardPaletteMenuItemString );}/**************************************************************************************************  BIMUpdateShowHideSendEventPaletteMenuItem**  Updates the "Show Send Event Palette"/"Hide Send Event Palette" menu item text as appropriate.**	In	inIsHidden		The current visible status of the send event palette.*************************************************************************************************/void BIMUpdateShowHideSendEventPaletteMenuItem( Boolean inIsHidden ){    if( inIsHidden )        SetMenuItemTextWithCFString( gPencilMenu, kShowHideSendEventPaletteMenuItem,                                     gShowSendEventPaletteMenuItemString );    else        SetMenuItemTextWithCFString( gPencilMenu, kShowHideSendEventPaletteMenuItem,                                     gHideSendEventPaletteMenuItemString );}#pragma mark -/**************************************************************************************************  BIMConvertToLowercase**  Converts the currently active text in the inline hole to lowercase. Called when the user*  selects the "Convert to Lowercase" menu item from our text service (pencil) menu.**	In	inSessionHandle		Our session context.**	Out	OSStatus		A toolbox result code.*************************************************************************************************/static OSStatus BIMConvertToLowercase( BIMSessionHandle inSessionHandle ){    UInt32 index;    UniChar character;        for( index = 0; index < ( *inSessionHandle )->fInputBufferCount; index++ ) {        character = ( *inSessionHandle )->fInputBuffer [index];        if( character >= 'A' && character <= 'Z' )            character += 0x20;        ( *inSessionHandle )->fInputBuffer [index] = character;    }    return BIMUpdateActiveInputArea( gActiveSession, false );}/**************************************************************************************************  BIMConvertToUppercase**  Converts the currently active text in the inline hole to uppercase. Called when the user*  selects the "Convert to Uppercase" menu item from our text service (pencil) menu.**	In	inSessionHandle		Our session context.**	Out	OSStatus		A toolbox result code.*************************************************************************************************/static OSStatus BIMConvertToUppercase( BIMSessionHandle inSessionHandle ){    UInt32 index;    UniChar character;        for( index = 0; index < ( *inSessionHandle )->fInputBufferCount; index++ ) {        character = ( *inSessionHandle )->fInputBuffer [index];        if( character >= 'a' && character <= 'z' )            character -= 0x20;        ( *inSessionHandle )->fInputBuffer [index] = character;    }    return BIMUpdateActiveInputArea( gActiveSession, false );}/**************************************************************************************************  BIMPencilMenuEventHandler**  Handles menu selection events in the pencil menu.**	In	inEventHandlerCallRef	A reference to this event handler.*		inEventRef		A reference to this event.*		inUserData		Contains our BIMSessionHandle that we passed as part of*					InstallMenuEventHandler().**	Out	OSStatus		eventNotHandledErr if the event was not handled, or*					noErr if the event was handled.*************************************************************************************************/static pascal OSStatus BIMPencilMenuEventHandler( EventHandlerCallRef inEventHandlerCallRef,						  EventRef inEventRef, void *inUserData ){    OSStatus result;    Boolean isHidden;    HICommand command;    //	Get the selected menu command.    result = GetEventParameter( inEventRef, kEventParamDirectObject, typeHICommand, nil,                               sizeof( command ), nil, &command);    if( result == noErr ) {        switch( command.commandID ) {                    //  Handle the "Show Keyboard Palette" or "Hide Keyboard Palette" menu item.                    case kShowHideKeyboardPaletteMenuCommand:                result = BIMIsKeyboardPaletteHidden( &isHidden );                if( result == noErr ) {                    if( isHidden )                        result = BIMShowKeyboardPalette();                    else                        result = BIMHideKeyboardPalette();                }                if( result == noErr )                    result = BIMIsKeyboardPaletteHidden( &isHidden );                if( result == noErr )                    BIMUpdateShowHideKeyboardPaletteMenuItem( isHidden );                break;                    //  Handle the "Show Send Event Palette" or "Hide Send Event Palette" menu item.                    case kShowHideSendEventPaletteMenuCommand:                result = BIMIsSendEventPaletteHidden( &isHidden );                if( result == noErr ) {                    if( isHidden )                        result = BIMShowSendEventPalette();                    else                        result = BIMHideSendEventPalette();                }                if( result == noErr )                    result = BIMIsSendEventPaletteHidden( &isHidden );                if( result == noErr )                    BIMUpdateShowHideSendEventPaletteMenuItem( isHidden );                break;                    //  Handle the "Convert to Lowercase" menu item.                    case kConvertToLowercaseMenuCommand:                if( gActiveSession )                    result = BIMConvertToLowercase( gActiveSession );                break;                    //  Handle the "Convert to Uppercase" menu item.                    case kConvertToUppercaseMenuCommand:                if( gActiveSession )                    result = BIMConvertToUppercase ( gActiveSession );                break;            default:                result = eventNotHandledErr;                break;        }    }    else        result = eventNotHandledErr;    return result;}/**************************************************************************************************  BIMLog**  Log the given message to stderr. Uncomment the following to enable logging.*************************************************************************************************/void BIMLog( unsigned char *message ){/*    fprintf( stderr, message );*/}