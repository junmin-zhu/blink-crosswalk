//
//  WebFileButton.m
//  WebKit
//
//  Created by Darin Adler on Tue Oct 22 2002.
//  Copyright (c) 2002 Apple Computer, Inc. All rights reserved.
//

#import "WebFileButton.h"

#import <WebKit/WebAssertions.h>
#import <WebKit/WebLocalizableStrings.h>
#import <WebKit/WebBridge.h>
#import <WebKit/WebStringTruncator.h>

#define NO_FILE_SELECTED 

#define AFTER_BUTTON_SPACING 4
#define ICON_HEIGHT 16
#define ICON_WIDTH 16
#define ICON_FILENAME_SPACING 2

// We empirically determined that buttons have these extra pixels on all
// sides. It would be better to get this info from AppKit somehow.
#define BUTTON_TOP_MARGIN 4
#define BUTTON_BOTTOM_MARGIN 6
#define BUTTON_LEFT_MARGIN 5
#define BUTTON_RIGHT_MARGIN 5

// AppKit calls this kThemePushButtonSmallTextOffset.
#define BUTTON_VERTICAL_FUDGE_FACTOR 2

@interface WebFileChooserButton : NSButton
{
    id <WebCoreFileButtonDelegate> _delegate;
}
- (id)initWithDelegate:(id <WebCoreFileButtonDelegate>)delegate;
@end

@implementation WebFileButton

- (void)positionButton
{
    [_button sizeToFit];
    [_button setFrameOrigin:NSMakePoint(0, 0)];
}

- (id)initWithBridge:(WebBridge *)bridge delegate:(id <WebCoreFileButtonDelegate>)delegate
{
    self = [super init];
    if (self) {
	_bridge = bridge; // Don't retain to avoid cycle
        _delegate = [delegate retain];
    }
    return self;
}

- (id)initWithFrame:(NSRect)frame
{
    self = [super initWithFrame:frame];
    if (self) {
        _button = [[WebFileChooserButton alloc] initWithDelegate:_delegate];
        
        [_button setTitle:UI_STRING("Choose File", "title for file button used in HTML forms")];
        [[_button cell] setControlSize:NSSmallControlSize];
        [_button setFont:[NSFont systemFontOfSize:[NSFont smallSystemFontSize]]];
        [_button setBezelStyle:NSRoundedBezelStyle];
        [_button setTarget:self];
        [_button setAction:@selector(chooseButtonPressed:)];
        [_button setNextResponder:self];
        
        [self addSubview:_button];
        
        [self positionButton];
        [self setFilename:nil];
    }
    return self;
}

- (void)dealloc
{
    [_filename release];
    [_button release];
    [_icon release];
    [_label release];
    [_delegate release];
    [super dealloc];
}

- (BOOL)isFlipped
{
    return YES;
}
        
- (void)drawRect:(NSRect)rect
{
    NSRect bounds = [self bounds];
    
    [NSGraphicsContext saveGraphicsState];
    NSRectClip(NSIntersectionRect(bounds, rect));

    float left = NSMaxX([_button frame]) + AFTER_BUTTON_SPACING;

    if (_icon) {
        float top = (bounds.size.height - BUTTON_BOTTOM_MARGIN - BUTTON_TOP_MARGIN - ICON_HEIGHT) / 2
            + BUTTON_TOP_MARGIN;
        [_icon drawInRect:NSMakeRect(left, top, ICON_WIDTH, ICON_HEIGHT)
            fromRect:NSMakeRect(0, 0, [_icon size].width, [_icon size].height)
            operation:NSCompositeSourceOver fraction:1.0];
        left += ICON_WIDTH + ICON_FILENAME_SPACING;
    }

    NSFont *font = [_button font];
    NSDictionary *attributes = [NSDictionary dictionaryWithObject:font forKey:NSFontAttributeName];
    [_label drawAtPoint:NSMakePoint(left, [self baseline] - [[_button font] ascender]) withAttributes:attributes];

    [NSGraphicsContext restoreGraphicsState];
}

- (void)updateLabel
{
    [_label release];
    
    NSString *label;
    if ([_filename length]) {
        label = _filename;
    } else {
        label = [UI_STRING("no file selected", "text to display in file button used in HTML forms when no file is selected") retain];
    }
    
    float left = NSMaxX([_button frame]) + AFTER_BUTTON_SPACING;
    if (_icon) {
        left += ICON_WIDTH + ICON_FILENAME_SPACING;
    }
    float labelWidth = [self bounds].size.width - left;

    _label = labelWidth <= 0 ? nil : [[WebStringTruncator centerTruncateString:
        [[NSFileManager defaultManager] displayNameAtPath:label]
        toWidth:labelWidth withFont:[_button font]] copy];
}

- (void)setFilename:(NSString *)filename
{
    NSString *copy = [filename copy];
    [_filename release];
    _filename = copy;
    
    [_icon release];
    if ([_filename length] == 0 || [_filename characterAtIndex:0] != '/') {
        _icon = nil;
    } else {
        _icon = [[[NSWorkspace sharedWorkspace] iconForFile:_filename] retain];
    }
    
    [self updateLabel];
    
    [self setNeedsDisplay:YES];
}

- (NSString *)filename
{
    return [[_filename copy] autorelease];
}

- (void)setFrameSize:(NSSize)size
{
    [super setFrameSize:size];
    // FIXME: Can we just springs and struts instead of calling positionButton?
    [self positionButton];
    [self updateLabel];
}

- (NSSize)bestVisualFrameSizeForCharacterCount:(int)count
{
    ASSERT(count > 0);
    NSSize size = [[_button cell] cellSize];
    size.height -= BUTTON_TOP_MARGIN + BUTTON_BOTTOM_MARGIN;
    size.width -= BUTTON_LEFT_MARGIN + BUTTON_RIGHT_MARGIN;
    size.width += AFTER_BUTTON_SPACING + ICON_WIDTH + ICON_FILENAME_SPACING;
    size.width += count * [[_button font] widthOfString:@"x"];
    return size;
}

- (NSRect)visualFrame
{
    ASSERT([self superview] == nil || [[self superview] isFlipped]);
    NSRect frame = [self frame];
    frame.origin.x += BUTTON_LEFT_MARGIN;
    frame.size.width -= BUTTON_LEFT_MARGIN;
    frame.origin.y += BUTTON_TOP_MARGIN;
    frame.size.height -= BUTTON_TOP_MARGIN + BUTTON_BOTTOM_MARGIN;
    return frame;
}

- (void)setVisualFrame:(NSRect)frame
{
    ASSERT([self superview] == nil || [[self superview] isFlipped]);
    frame.origin.x -= BUTTON_LEFT_MARGIN;
    frame.size.width += BUTTON_LEFT_MARGIN;
    frame.origin.y -= BUTTON_TOP_MARGIN;
    frame.size.height += BUTTON_TOP_MARGIN + BUTTON_BOTTOM_MARGIN;
    [self setFrame:frame];
}

- (float)baseline
{
    // Button text is centered vertically, with a fudge factor to account for the shadow.
    ASSERT(_button);
    NSFont *buttonFont = [_button font];
    float ascender = [buttonFont ascender];
    float descender = [buttonFont descender];
    return -BUTTON_TOP_MARGIN
        + ([[_button cell] cellSize].height + BUTTON_TOP_MARGIN + BUTTON_BOTTOM_MARGIN - (ascender - descender)) / 2.0
        + ascender - BUTTON_VERTICAL_FUDGE_FACTOR;
}

- (void)beginSheet
{
    [_bridge retain];
    [_bridge runOpenPanelForFileButtonWithResultListener:self];
}

- (void)chooseFilename:(NSString *)fileName
{
    [self setFilename:fileName];
    [_delegate filenameChanged:fileName];
    [_bridge release];
}

- (void)cancel
{
    [_bridge release];
}


- (void)chooseButtonPressed:(id)sender
{
    [_delegate clicked];
    [self beginSheet];
}

- (void)mouseDown:(NSEvent *)event
{
    [self beginSheet];
}

- (BOOL)acceptsFirstResponder
{
    return YES;
}

- (BOOL)becomeFirstResponder
{
    BOOL become = [_button acceptsFirstResponder];
    if (become) {
        [_delegate focusChanged:YES];
        [[self window] makeFirstResponder:_button];
    }
    return become;
}

- (NSView *)nextKeyView
{
    return (_inNextValidKeyView && ![_bridge inNextKeyViewOutsideWebFrameViews])
    ? [_bridge nextKeyView]
    : [super nextKeyView];
}

- (NSView *)previousKeyView
{
    return (_inNextValidKeyView)
    ? [_bridge previousKeyView]
    : [super previousKeyView];
}

- (NSView *)nextValidKeyView
{
    _inNextValidKeyView = YES;
    NSView *view = [super nextValidKeyView];
    _inNextValidKeyView = NO;
    return view;
}

- (NSView *)previousValidKeyView
{
    _inNextValidKeyView = YES;
    NSView *view = [super previousValidKeyView];
    _inNextValidKeyView = NO;
    return view;
}

@end

@implementation WebFileChooserButton

- (id)initWithDelegate:(id <WebCoreFileButtonDelegate>)delegate
{
    [super init];
    _delegate = delegate; // Don't retain to avoid cycle
    return self;
}

- (NSView *)nextValidKeyView
{
    return [[self superview] nextValidKeyView];
}

- (NSView *)previousValidKeyView
{
    return [[self superview] previousValidKeyView];
}

- (BOOL)resignFirstResponder
{
    [_delegate focusChanged:NO];
    return YES;
}

@end
