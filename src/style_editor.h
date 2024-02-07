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

#include <vector>

#include <wx/wx.h>
#include <wx/clrpicker.h>
#include <wx/gbsizer.h>
#include <wx/propgrid/propgrid.h>

wxDECLARE_EVENT(STYLE_UPDATED, wxCommandEvent);
wxDECLARE_EVENT(NUMBER_CENTROIDS_CHANGED, wxCommandEvent);

/** Manages style information for drawing the tree. */
class StyleEditor : public wxFrame {
public:
    StyleEditor(wxWindow *parent);

    /** Color of 'regular' Nodes (i.e., neither root nor centroids) */
    wxColor GetDefaultColor() const;

    /** Color of the root Node. */
    wxColor GetRootColor() const;

    /** Color of centroid with given centrioid_id. */
    wxColor GetColorForCentroid(int centroid_id) const;


    /** Get the number of centroids. */
    int GetNumberCentroids() const;

    /** Set number of centroids property. */
    void SetNumberCentroids(int n);

private:

    /** Emit signal indicating that one or more of the colors were changed. */
    void NotifyColorsChanged();

    /** Eit signal indicating that the number of centroids has changed. */
    void NotifyNumberCentroidsChanged();

    /** Hide on close rather than delete. */
    void OnCloseWindow(wxCloseEvent &evt) { Show(false); }

    void OnPropertyGridChanged(wxPropertyGridEvent &evt);
    //void OnResetClicked(wxCommandEvent &evt);
    void SetDefaults();

    wxPropertyGrid *propgrid_ = nullptr;

};

/** Manages style information for drawing the tree.
class StyleEditor : public wxFrame {
public:
    StyleEditor(wxWindow *parent);

    wxColor GetDefaultColor() const { return default_clrpkr_->GetColour(); }

    wxColor GetRootColor()    const { return root_clrpkr_->GetColour(); }

    wxColor GetColorForCentroid(int centroid_id) const;

private:

    void NotifyColorsChanged();

    void OnCloseWindow(wxCloseEvent &evt) { Show(false); }

    void OnColourPicked(wxColourPickerEvent &evt);
    void OnResetClicked(wxCommandEvent &evt);
    void SetDefaults();

    wxPropertyGrid *propgrid_ = nullptr;
    
    wxScrolledWindow   *scrolled_       = nullptr;
    wxColourPickerCtrl *default_clrpkr_ = nullptr;
    wxColourPickerCtrl *root_clrpkr_    = nullptr;

    std::vector<wxColourPickerCtrl *> centroid_clrpkrs_; //map centroid_id (vector index) to controls
};
*/