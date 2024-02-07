/*
Copyright 2024, The Broad Institute of MIT and Harvard

Original Author: Charles C Bailey

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights to
use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies
of the Software, and to permit persons to whom the Software is furnished to do
so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#ifndef CCB_CANVAS_H_
#define CCB_CANVAS_H_

#include <memory>
#include <optional>

#include <wx/wx.h>
#include <wx/dcsvg.h>
#include <wx/graphics.h>
#include <wx/timer.h>
#include <wx/tipwin.h>

#include "network.h"

class Canvas;
struct Network;
class wxBufferedPaintDC;

class MonospaceWindow : public wxWindow {
    public: MonospaceWindow(wxWindow *parent, const wxFont &font) : wxWindow(parent, wxID_ANY) { SetFont(font); }
};

class MonospaceTipWindow : public MonospaceWindow, public wxTipWindow {
public:
    MonospaceTipWindow(wxWindow *parent, const wxString &text, const wxFont &font, wxTipWindow **ptr=nullptr)
    : MonospaceWindow(parent, font)
    , wxTipWindow(parent, text, 100000, ptr) {}
};

/** Event emitted when the Canvas has enabled or disabled auto tracking,
* for example, when the user drags a Node.
*/
wxDECLARE_EVENT(AUTO_TRACK_CHANGED, wxCommandEvent);

/** This poor widget is tasked with displaying a Network and
* animating its physical simulation. Can also be painted to a
* wxImage or wxSVGFileDC for raster or vector output, respectively.
*/
class Canvas : public wxPanel {
public:
    /** wxPanel constructor */
    Canvas(wxFrame *parent);

    /** Provide an instance of a Network object.
    * Also resets the the default viewing coordinates.
    * Setting a null pointer disables user interaction.
    */
    void SetNetwork(std::shared_ptr<Network>);

    /** Get the Network instance. */
    std::shared_ptr<Network> GetNetwork() { return net_; }

    /** Start a wxTimer that updates the Network simulation and repaints us. */
    void StartAnimation();

    /** Stop animating. */
    void StopAnimation();

    /** Check if Canvas is auto tracking. */
    bool GetAutoTrack() const { return auto_track_; }

    /** Auto pan and zoom to fit Network on Canvas. */
    void SetAutoTrack(bool on_off) { auto_track_=on_off; if (!animation_timer_.IsRunning()) Refresh(); }

    /** Render to SVG file. */
    void PaintSVG(wxSVGFileDC &dc);

    /** Render to raster image. */
    void PaintImage(wxImage &img);

    /** Request immediate repaint. */
    void PaintNow();

    /** Render to our own window using an anti-aliased wxGraphicsContext. */
    void render(std::unique_ptr<wxGraphicsContext> &&);
    
private:
    //Emit AUTO_TRACK_CHANGED
    void NotifyAutoTrackChanged();

    //Translate local window coordinates to Network coordinates
    Vec2 ClientToNetwork(wxCoord x, wxCoord y);

    //Translate screen coordinates to Network coordinates
    Vec2 ScreenToNetwork(wxCoord x, wxCoord y);

    //IDs for the animation_timer_ and the tooltip_timer_ 
    enum TIMER_IDS : int {
        ANIMATION_TIMER_ID = 1,
        TOOLTIP_TIMER_ID
    };

    std::shared_ptr<Network> net_; //pointer to Network object
    wxGraphicsMatrix transform_;   //current client coords->network coords transformation matrix

    int svg_scale_factor_ = 128;   //when exporting to svg we scale all coords to get around integer artithmetic in wxDC

    double x_min_ = -1.0;          //bounding box for our network in network coordinates
    double x_max_ =  1.0;
    double y_min_ = -1.0;
    double y_max_ =  1.0;

    double init_x_min_ = -1.0;     //position of origin when user initiates pan
    double init_y_min_ = -1.0;

    double init_sf_ = 1.0;         //the scale factor saved when the user initiates zoom in/out
    double sf_ = 1.0;              //the current scale factor
    bool auto_track_ = true;       //if true, camera will pan and zoom to fit entire network on screen

    size_t iterations_per_frame_ = 3; //number of steps to simulate between draw calls
    wxTimer animation_timer_;         //periodically update and paint simulation
    wxTimer tooltip_timer_;           //delay on hover before tooltip is shown

    Node *picked_ = nullptr;               //the node selected by the user with LMB click or hovered over
    wxRect mouse_trap_;                    //when mouse leaves this rectangle, any active tooltip should disappear
    std::optional<Vec2> click_pos_net_;    //store location of LMB down in network space
    std::optional<wxPoint> click_pos_cli_; //store location of LMB down in client space

    std::optional<Vec2> ptr_pos_net_;      //store last location of mouse pointer in network space
    std::optional<wxPoint> ptr_pos_cli_;   //store last location of mouse pointer in client space

    wxTipWindow *tip_window_ = nullptr; //pointer to active tooltip

    void OnPaintEvent(wxPaintEvent &evt);
    void OnLeftDownEvent(wxMouseEvent &evt);
    void OnLeftUpEvent(wxMouseEvent &evt);
    void OnMouseMoving(wxMouseEvent &evt);
    void OnTimerEvent(wxTimerEvent &evt);
    void OnMouseWheel(wxMouseEvent &evt);
    //void OnGestureZoom(wxZoomGestureEvent &evt);
};

#endif