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

#include "canvas.h"
#include "network.h"

#include <wx/dcbuffer.h>
#include <wx/graphics.h>

wxDEFINE_EVENT(AUTO_TRACK_CHANGED, wxCommandEvent);

/*
MonospaceTipWindow::MonospaceTipWindow(wxWindow *parent, const wxString &text, const wxFont &font, wxTipWindow **ptr) {

}
*/

Canvas::Canvas(wxFrame *parent)
    : wxPanel(parent, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxFULL_REPAINT_ON_RESIZE)
    , transform_()
    , animation_timer_(this, ANIMATION_TIMER_ID)
    , tooltip_timer_(this, TOOLTIP_TIMER_ID) {
    SetBackgroundStyle(wxBG_STYLE_PAINT);
    SetSizeHints(wxSize(512, 512));
    //EnableTouchEvents(wxTOUCH_ZOOM_GESTURE);
    Bind(wxEVT_PAINT, &Canvas::OnPaintEvent, this);
}

void
Canvas::SetNetwork(std::shared_ptr<Network> net) { 
    StopAnimation();
    tooltip_timer_.Stop();

    net_ = net;

    click_pos_cli_ = std::nullopt;
    click_pos_net_ = std::nullopt;
    ptr_pos_cli_ = std::nullopt;
    ptr_pos_net_ = std::nullopt;
    picked_ = nullptr;

    double x_min_ = -1.0;          //bounding box for our network in network coordinates
    double x_max_ = 1.0;
    double y_min_ = -1.0;
    double y_max_ = 1.0;

    double init_x_min_ = -1.0;     //position of origin when user initiates pan
    double init_y_min_ = -1.0;

    double init_sf_ = 1.0;         //the scale factor saved when the user initiates zoom in/out
    double sf_ = 1.0;              //the current scale factor
    bool auto_track_ = true;

    if (HasCapture()) ReleaseMouse();

    if (net_) {
        Bind(wxEVT_LEFT_DOWN,    &Canvas::OnLeftDownEvent, this);
        Bind(wxEVT_LEFT_UP,      &Canvas::OnLeftUpEvent,   this);
        Bind(wxEVT_MOTION,       &Canvas::OnMouseMoving,   this);
        Bind(wxEVT_TIMER,        &Canvas::OnTimerEvent,    this);
        //Bind(wxEVT_GESTURE_ZOOM, &Canvas::OnGestureZoom,   this);
        Bind(wxEVT_MOUSEWHEEL,   &Canvas::OnMouseWheel,    this);
    } else {
        Unbind(wxEVT_LEFT_DOWN,    &Canvas::OnLeftDownEvent, this);
        Unbind(wxEVT_LEFT_UP,      &Canvas::OnLeftUpEvent,   this);
        Unbind(wxEVT_MOTION,       &Canvas::OnMouseMoving,   this);
        Unbind(wxEVT_TIMER,        &Canvas::OnTimerEvent,    this);
        //Unbind(wxEVT_GESTURE_ZOOM, &Canvas::OnGestureZoom,   this);
        Unbind(wxEVT_MOUSEWHEEL,   &Canvas::OnMouseWheel,    this);
    }

    Refresh();
}

void
Canvas::StartAnimation() { 
    animation_timer_.Start();
}

void
Canvas::StopAnimation() { 
    animation_timer_.Stop();
}

void
Canvas::OnPaintEvent(wxPaintEvent &evt) {
    wxBufferedPaintDC dc(this);
    std::unique_ptr<wxGraphicsContext> gc(wxGraphicsContext::Create(dc));
    render(std::move(gc));
}

void
Canvas::OnLeftDownEvent(wxMouseEvent &evt) {
    //clear and update our saved mouse positions
    picked_ = nullptr;
    click_pos_net_ = std::nullopt;
    click_pos_cli_ = std::nullopt;

    click_pos_cli_ = wxPoint(evt.GetX(), evt.GetY());
    Vec2 pos_net = ClientToNetwork(evt.GetX(), evt.GetY());
    click_pos_net_ = pos_net;

    ptr_pos_cli_ = std::nullopt;
    ptr_pos_net_ = std::nullopt;

    init_x_min_ = x_min_;
    init_y_min_ = y_min_;

    picked_ = net_->pick(pos_net);
    if (picked_) {
        std::cout << picked_->aas() << std::endl;
        if (false != auto_track_) {
            auto_track_ = false;
            NotifyAutoTrackChanged();
        }
        net_->pin_node(picked_->id());
    }

    //capture mouse so we can drag and pan when pointer leaves Canvas
    CaptureMouse(); 
}

void
Canvas::OnLeftUpEvent(wxMouseEvent &evt) {
    if (picked_) {
        net_->unpin_node(picked_->id());
        picked_ = nullptr;
    }
    click_pos_cli_ = std::nullopt;
    click_pos_net_ = std::nullopt;
    ptr_pos_cli_   = std::nullopt;
    ptr_pos_net_   = std::nullopt; 

    if (HasCapture()) ReleaseMouse();
}

void Canvas::OnMouseMoving(wxMouseEvent &evt) {
    Vec2 pos_net = ClientToNetwork(evt.GetX(), evt.GetY());

    if (!ptr_pos_cli_) ptr_pos_cli_ = click_pos_cli_;
    if (!ptr_pos_net_) ptr_pos_net_ = click_pos_net_;

    //wxWidget's mouse drag information doesn't seem to work so we
    //we use click_pos_cli_ to track if button 1 is still down
    if (click_pos_cli_) {
        if (false != auto_track_) {
            auto_track_ = false;
            NotifyAutoTrackChanged();
        }
        if (picked_) { //translate a Node if user clicked one
            double dx_net = pos_net.x - ptr_pos_net_->x;
            double dy_net = pos_net.y - ptr_pos_net_->y;
            net_->translate_node(picked_->id(), dx_net, dy_net);
        } else { //otherwise pan the camera
            int dx_cli = ptr_pos_cli_->x - click_pos_cli_->x;
            int dy_cli = ptr_pos_cli_->y - click_pos_cli_->y;
            x_min_ = init_x_min_ - dx_cli / sf_;
            y_min_ = init_y_min_ - dy_cli / sf_;
        }
        if (!animation_timer_.IsRunning()) Refresh();
    }
    ptr_pos_cli_ = wxPoint(evt.GetX(), evt.GetY());
    ptr_pos_net_ = Vec2(pos_net.x, pos_net.y);

    //every time we move the pointer we create mouse_trap_ box around it
    //and reset the tooltip_timer_
    //if the pointer is still in mouse_trap_ when tooltip_timer_ fires
    //we show a tooltip
    wxPoint mpos = ::wxGetMousePosition();
    if (tip_window_ && mouse_trap_.GetWidth() && !mouse_trap_.Contains(mpos)) tip_window_->Close();

    tooltip_timer_.Stop();
    tooltip_timer_.StartOnce(400);
}

void recursize_set_font(wxWindow *window, const wxFont &font) {
    if (!window) return;
    window->SetFont(font);
    for (wxWindow *child : window->GetChildren()) recursize_set_font(child, font);
    window->Layout();
}

void
Canvas::OnTimerEvent(wxTimerEvent &evt) {
    if (evt.GetTimer().GetId() == ANIMATION_TIMER_ID) {
        if (net_) {
            for (size_t i=0; i != iterations_per_frame_; ++i) net_->simulate_step();
        }
        Refresh();
        Update();
        wxYieldIfNeeded();
    } else if (evt.GetTimer().GetId() == TOOLTIP_TIMER_ID) {
        if (tip_window_) tip_window_->Close(); //close existing tooltip if any
        if (!tip_window_) {
            wxPoint pos_scr = ::wxGetMousePosition();
            Vec2 pos_net = ScreenToNetwork(pos_scr.x, pos_scr.y);
            Node *n = net_->pick(Vec2(pos_net.x, pos_net.y)); //see if there's a node under the cursor
            if (n) { //if so, show its mutations in a tooltip
                mouse_trap_ = wxRect(pos_scr.x - 5, pos_scr.x - 5, 10, 10);

                //The following code is an ugly hack to work around the fact that wxTipWindow uses a special
                //method to caluclate its client size and doesn't update when the font is changed. Methods
                //like Layout(), Update(), etc. also won't work. So we manually calculate the size of the tooltip
                //and manually call SetClientSize on the wxTipWindow's children (of which it has one, the privately
                //implemented wxTipWindowView that displays the tooltip text) via a generic wxWindow pointer.

                //create the wxTipWindow and give it and its wxTipWindowView child a fixed with font
                tip_window_ = new wxTipWindow(this, n->style.tooltip, 1000000, &tip_window_);
                wxFont monospace(wxSize(12, 12), wxFontFamily::wxFONTFAMILY_TELETYPE, wxFontStyle::wxFONTSTYLE_NORMAL, wxFontWeight::wxFONTWEIGHT_NORMAL);
                tip_window_->SetFont(monospace);
                for (wxWindow *child : tip_window_->GetChildren()) child->SetFont(monospace);

                //wxWidgets text extents methods don't deal with multiline text so we have to do the
                //calculation line-by-line
                std::string tooltip = n->style.tooltip.ToStdString();
                std::vector<std::string_view> lines = split(tooltip, "\n");

                int maxw = 0, maxh = 0;
                for (const auto &line : lines) {
                    wxSize extents = tip_window_->GetTextExtent(wxString(line.data(), line.data() + line.size()));
                    if (extents.GetWidth() > maxw) maxw = extents.GetWidth();
                    if (extents.GetHeight() > maxh) maxh = extents.GetHeight();
                }

                //the raw calculation is still a bit too small for some reason so we add some padding
                const int PADDING = tip_window_->GetTextExtent("X").GetHeight();

                //finally, we set the sizes of the wxTipWindow and its secret wxTipWindowView child
                tip_window_->SetClientSize(PADDING + maxw, PADDING + maxh*(1 + lines.size()));
                for (wxWindow *child : tip_window_->GetChildren()) child->SetClientSize(PADDING + maxw, PADDING + maxh*(1 + lines.size()));
            }
        }
    }
}

void
Canvas::OnMouseWheel(wxMouseEvent &evt) {
    //Windows maps pinch-to-zoom to ctrl+mouse wheel for some reason
    if (evt.ControlDown()) { 
        if (false != auto_track_) {
            auto_track_ = false;
            NotifyAutoTrackChanged();
        }
        int delta = evt.GetWheelDelta();
        int rot = evt.GetWheelRotation();
        float turns = std::fabs(rot / static_cast<float>(delta));
        if (rot < 0) 
            sf_ *= powf(0.9f, turns);
        else
            sf_ *= powf(1.1f, turns);
    }
    if (!animation_timer_.IsRunning()) Refresh();
}

/*
void
Canvas::OnGestureZoom(wxZoomGestureEvent &evt) {
    if (evt.IsGestureStart()) {
        init_sf_ = sf_;
        if (false != auto_track_) {
            auto_track_ = false;
            NotifyAutoTrackChanged();
        }
    } else if (evt.IsGestureEnd()) {
        init_sf_ = 1.0;
    }
    sf_ = init_sf_ * evt.GetZoomFactor();
    if (!animation_timer_.IsRunning()) Refresh();
}
*/

void
Canvas::PaintImage(wxImage &img) {
    bool was_animating_ = animation_timer_.IsRunning();
    bool was_tracking_ = auto_track_;

    StopAnimation();
    auto_track_ = true;

    std::unique_ptr<wxGraphicsContext> gc(wxGraphicsContext::Create(img));
    render(std::move(gc));

    auto_track_ = was_tracking_;
    if (was_animating_) animation_timer_.Start();
}

void
draw_edge(wxGraphicsContext *gc, Node *n) {
    if (n->is_root()) return;
    gc->SetPen(n->style.edge_pen);
    gc->StrokeLine(n->pos.x, n->pos.y, n->parent()->pos.x, n->parent()->pos.y);
}

void
draw_edge(wxSVGFileDC *gc, Node *n, int sf) {
    if (n->is_root()) return;
    wxPen pen = n->style.edge_pen;
    pen.SetWidth(pen.GetWidth() ? pen.GetWidth() * sf : std::max(1, sf / 5));
    gc->SetPen(pen);
    gc->DrawLine(n->pos.x, n->pos.y, n->parent()->pos.x, n->parent()->pos.y);
}

void
draw_node(wxGraphicsContext *gc, Node *n, double sf, double x_min, double y_min) {
    gc->SetPen(n->style.pen);
    gc->SetBrush(n->style.brush);
    gc->DrawEllipse(n->pos.x - n->r, n->pos.y - n->r, 2 * n->r, 2 * n->r);

    if (!n->style.label.empty()) {
        gc->PushState();
        gc->SetTransform(gc->CreateMatrix());

        double w, h, d, e;
        gc->GetTextExtent(n->style.label, &w, &h, &d, &e);
        double tsf = std::min(2 * n->r / w, 2 * n->r / h);
        double hpad = (2 * n->r - tsf * w) / 2;
        double vpad = (2 * n->r - tsf * (h - d / 2)) / 2;

        gc->Scale(sf, sf);
        gc->Translate(-x_min + n->pos.x - n->r + hpad, -y_min + n->pos.y - n->r - vpad);
        gc->Scale(tsf, tsf);

        gc->DrawText(n->style.label, 0, 0);
        gc->PopState();
    }
}

void
draw_node(wxSVGFileDC *gc, Node *n, int sf, double x_min, double y_min) {
    wxPen pen = n->style.pen;
    pen.SetWidth(pen.GetWidth() ? pen.GetWidth() * sf : std::max(1, sf / 5));
    gc->SetPen(pen);
    gc->SetBrush(n->style.brush);
    gc->DrawEllipse(n->pos.x - n->r, n->pos.y - n->r, 2 * n->r, 2 * n->r);

    if (!n->style.label.empty()) {
        wxCoord w, h, d, e;
        gc->GetTextExtent(n->style.label, &w, &h, &d, &e);

        static const double root_2 = 1.4142135623730951;
        const double box_x = n->pos.x - n->r / root_2;
        const double box_y = n->pos.y - n->r / root_2;
        const double box_w = 2 * n->r / root_2;
        const double box_h = 2 * n->r / root_2;

        const double tsf = std::min(box_w / w, box_h / h);

        wxFont current_font = gc->GetFont();
        wxFont scaled_font(current_font);
        scaled_font.Scale(tsf);
        gc->SetFont(scaled_font);
        gc->GetTextExtent(n->style.label, &w, &h, &d, &e);

        const double hpad = (box_w - w) / 2;
        const double vpad = (box_h - h) / 2;
        const double tx = n->pos.x - box_w / 2 + hpad;
        const double ty = n->pos.y - box_h / 2 + vpad - d /2.0;

        gc->DrawText(n->style.label, tx, ty);
        gc->SetFont(current_font);
    }
}

void
Canvas::PaintSVG(wxSVGFileDC &dc) {
    wxSVGFileDC *gc = &dc;

    gc->SetBrush(*wxWHITE_BRUSH);
    gc->SetFont(*wxNORMAL_FONT);

    //double w = 4096, h = 4096;

    if (!net_) return;

    //wxWidgets has integer arithmetic embedded in its Pen and DrawText methods that
    //makes ugly artifacts in wxSVGFileDC (super thick lines and off-center text)
    //so we use this awful hack to essentially "oversample" the image by scaling the
    //entire Network coordinate system before painting...
    float net_x_min = 0.0f;
    float net_y_min = 0.0f;
    float net_x_max = 1.0f;
    float net_y_max = 1.0f;

    if (!net_->nodes().empty()) {
        const Node &n = net_->begin()->second;
        net_x_min = net_x_max = n.pos.x;
        net_y_min = net_y_max = n.pos.y;
    }

    for (auto &[_, n] : *net_) {
        if (n.pos.x < net_x_min) net_x_min = n.pos.x;
        if (n.pos.x > net_x_max) net_x_max = n.pos.x;
        if (n.pos.y < net_y_min) net_y_min = n.pos.y;
        if (n.pos.y > net_y_max) net_y_max = n.pos.y;
    }

    if (net_x_max - net_x_min < 1.0) {
        float m = (net_x_min + net_x_max)/2.0f;
        net_x_min = m - 0.5f;
        net_x_max = m + 0.5f;
    }

    if (net_y_max - net_y_min < 1.0) {
        float m = (net_y_min + net_y_max)/2.0f;
        net_y_min = m - 0.5f;
        net_y_max = m + 0.5f;
    }

    for (auto &[_, n] : *net_) {
        if (n.pos.x < net_x_min) net_x_min = n.pos.x;
        if (n.pos.x > net_x_max) net_x_max = n.pos.x;
        if (n.pos.y < net_y_min) net_y_min = n.pos.y;
        if (n.pos.y > net_y_max) net_y_max = n.pos.y;
        n.pos.x *= svg_scale_factor_;
        n.pos.y *= svg_scale_factor_;
        n.r     *= svg_scale_factor_;
    }

    float net_w = net_x_max - net_x_min;
    float net_h = net_y_max - net_y_min;

    //gc->SetLogicalScale(1.0f / svg_scale_factor_, 1.0f / svg_scale_factor_);
    gc->SetLogicalOrigin(net_x_min * svg_scale_factor_, net_y_min * svg_scale_factor_);


    //gc->SetLogicalScale(sf_ / svg_scale_factor_, sf_ / svg_scale_factor_);
    //gc->SetLogicalOrigin(x_min_, y_min_);

    const std::vector<Node *> &nodes = net_->z_ordered_nodes();
    size_t i = 0, j = 0;

    //draw edges at z = -1
    for (; i != nodes.size() && nodes[i]->style.z == Node::NA; ++i) {
        draw_edge(gc, nodes[i], svg_scale_factor_);
    }

    //draw nodes at z = -1
    for (; j != i; ++j) {
        draw_node(gc, nodes[j], svg_scale_factor_, x_min_, y_min_);
    }

    //draw centroid root paths in z-order
    for (; i != nodes.size(); ++i) {
        draw_edge(gc, nodes[i], svg_scale_factor_);
    }

    //draw centroids and root path nodes in z order
    for (; j != i; ++j) {
        draw_node(gc, nodes[j], svg_scale_factor_, x_min_, y_min_);
    }

    //remember to put our coordinates back where we found them and hope
    //nobody noticed what we did...
    for (auto &[_, n] : *net_) {
        n.pos.x /= svg_scale_factor_;
        n.pos.y /= svg_scale_factor_;
        n.r     /= svg_scale_factor_;
    }
}

void
Canvas::render(std::unique_ptr<wxGraphicsContext> &&gc) {
    if (!gc) return;

    gc->SetBrush(*wxWHITE_BRUSH);
    gc->SetFont(*wxNORMAL_FONT, wxColor(*wxBLACK));

    double w, h;
    gc->GetSize(&w, &h);
    gc->DrawRectangle(0.0, 0.0, w, h);

    if (!net_ || net_->nodes().empty()) return;

    if (auto_track_) {
        Node &root = net_->node(0);
        x_min_ = root.pos.x - root.r;
        y_min_ = root.pos.y - root.r;
        x_max_ = root.pos.x + root.r;
        y_max_ = root.pos.y + root.r;

        for (auto &[_, n] : net_->nodes()) {
            if (n.pos.x - n.r < x_min_) x_min_ = n.pos.x - n.r;
            if (n.pos.y - n.r < y_min_) y_min_ = n.pos.y - n.r;
            if (n.pos.x + n.r > x_max_) x_max_ = n.pos.x + n.r;
            if (n.pos.y + n.r > y_max_) y_max_ = n.pos.y + n.r;
        }

        const double SF_MIN = 0.0001;
        double sx = w / std::max(x_max_ - x_min_, SF_MIN);
        double sy = h / std::max(y_max_ - y_min_, SF_MIN);

        sf_ = std::min(sx, sy);
    }

    gc->Scale(sf_, sf_);
    gc->Translate(-x_min_, -y_min_);
    transform_ = gc->GetTransform();

    const std::vector<Node *> &nodes = net_->z_ordered_nodes();
    size_t i=0, j=0;

    //draw edges at z = -1
    for (; i != nodes.size() && nodes[i]->style.z == Node::NA; ++i) {
        draw_edge(gc.get(), nodes[i]);
    }

    //draw nodes at z = -1
    for (; j != i; ++j) {
        draw_node(gc.get(), nodes[j], sf_, x_min_, y_min_);
    }

    //draw centroid root paths in z-order
    for (; i != nodes.size(); ++i) {
        draw_edge(gc.get(), nodes[i]);
    }

    //draw centroids and root path nodes in z order
    for (; j != i; ++j) {
        draw_node(gc.get(), nodes[j], sf_, x_min_, y_min_);
    }
}

void
Canvas::NotifyAutoTrackChanged() {
    wxCommandEvent evt(AUTO_TRACK_CHANGED);
    evt.SetInt(GetAutoTrack());
    wxPostEvent(this, evt);
}

void Canvas::PaintNow() {
    wxBufferedPaintDC dc(this);
    std::unique_ptr<wxGraphicsContext> gc(wxGraphicsContext::Create(dc));
    render(std::move(gc));
    Refresh();
}

Vec2 Canvas::ClientToNetwork(wxCoord x_cli, wxCoord y_cli) {
    wxGraphicsMatrix inv = transform_;
    if (!inv.GetMatrixData()) { //sometimes GetMatrixData() returns null shortly after program launch so we just "ignore" it
        //wxLogMessage("It happened again.");
        return Vec2(0, 0);
    }
    inv.Invert();
    double x_net = x_cli, y_net = y_cli;
    inv.TransformPoint(&x_net, &y_net);
    return Vec2(x_net, y_net);
}

Vec2 Canvas::ScreenToNetwork(wxCoord x, wxCoord y) {
    ScreenToClient(&x, &y);
    return ClientToNetwork(x, y);
}