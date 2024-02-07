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

#include <wx/event.h>
#include <wx/scrolwin.h>
#include <wx/stattext.h>
#include <wx/valnum.h>
#include <wx/window.h>
#include <wx/propgrid/propgrid.h>
#include <wx/propgrid/advprops.h>

#include "style_editor.h"
#include "util.h"

wxDEFINE_EVENT(STYLE_UPDATED, wxCommandEvent);
wxDEFINE_EVENT(NUMBER_CENTROIDS_CHANGED, wxCommandEvent);

StyleEditor::StyleEditor(wxWindow *parent)
    : wxFrame(parent, wxID_ANY, "Style Editor") {
    propgrid_ = new wxPropertyGrid(this);

    propgrid_->Append(new wxPropertyCategory("Centroids"));

    wxIntProperty *n_centroids_prop = new wxIntProperty("Number Centroids", wxPG_LABEL, 1);
    wxIntegerValidator<int> validator;
    validator.SetMin(0);
    n_centroids_prop->SetValidator(validator);
    propgrid_->Append(n_centroids_prop);

    propgrid_->Append(new wxPropertyCategory("Colors"));
    propgrid_->Append(new wxColourProperty("Root", wxPG_LABEL, wxColor("#000000")));
    propgrid_->Append(new wxColourProperty("Default", wxPG_LABEL, wxColor("#ffffff")));
    for (int i=0; i < PALETTE.size(); ++i) {
        wxString name = wxString::Format("Centroid %d", i + 1);
        propgrid_->Append(new wxColourProperty(name.c_str(), wxPG_LABEL, wxColor(PALETTE[i])));
    }

    propgrid_->FitColumns();
    Layout();

    Bind(wxEVT_PG_CHANGED,   &StyleEditor::OnPropertyGridChanged, this); 
    Bind(wxEVT_CLOSE_WINDOW, &StyleEditor::OnCloseWindow,         this);
}

wxColor StyleEditor::GetDefaultColor() const {
    wxAny value = propgrid_->GetProperty("Default")->GetValue();
    return value.As<wxColor>();
}

wxColor StyleEditor::GetRootColor() const {
    wxAny value = propgrid_->GetProperty("Root")->GetValue();
    return value.As<wxColor>();
}

wxColor
StyleEditor::GetColorForCentroid(int i) const {
    wxString name = wxString::Format("Centroid %zu", i % PALETTE.size() + 1);
    wxAny value = propgrid_->GetProperty(name)->GetValue();
    return value.As<wxColor>();
}

int StyleEditor::GetNumberCentroids() const {
    wxAny value = propgrid_->GetProperty("Number Centroids")->GetValue();
    int n = value.As<int>();
    return std::max(0, n);
}

void
StyleEditor::SetNumberCentroids(int n) {
    propgrid_->GetProperty("Number Centroids")->SetValue(wxVariant(n));
}

void
StyleEditor::OnPropertyGridChanged(wxPropertyGridEvent &evt) { 
    wxPGProperty *property = evt.GetProperty();

    if (!property) return;

    if (property->GetName() == "Number Centroids") {
        NotifyNumberCentroidsChanged();
    }

    if (property->GetName() == "Root" ||
        property->GetName() == "Default" ||
        property->GetName().StartsWith("Centroid ")) {
        NotifyColorsChanged();
    }
}

void
StyleEditor::NotifyColorsChanged() {
    wxCommandEvent evt(STYLE_UPDATED);
    wxPostEvent(this, evt);
}

void
StyleEditor::NotifyNumberCentroidsChanged() {
    wxCommandEvent evt(NUMBER_CENTROIDS_CHANGED);
    wxPostEvent(this, evt);
}

void
StyleEditor::SetDefaults() {
    wxColor color("#000000");
    wxVariant variant;

    propgrid_->GetProperty("Root")->SetValue(variant << color);


    color = "#ffffff";
    propgrid_->GetProperty("Default")->SetValue(variant << color);

    for (size_t i = 0; i != PALETTE.size(); ++i) {
        wxString name = wxString::Format("Centroid %d", i + 1);

        color = wxColor(PALETTE[i]);
        propgrid_->GetProperty(name)->SetValue(variant << color);
    }
}

/*
StyleEditor::StyleEditor(wxWindow *parent)
    : wxFrame(parent, wxID_ANY, "Style Editor") {
    scrolled_ = new wxScrolledWindow(this); 

    wxGridBagSizer *grid = new wxGridBagSizer();

    default_clrpkr_ = new wxColourPickerCtrl(scrolled_, wxID_ANY);
    root_clrpkr_    = new wxColourPickerCtrl(scrolled_, wxID_ANY);

    int row = 0;
    grid->Add(new wxStaticText(scrolled_, wxID_ANY, "Default"), {row,0}, {1,1});
    grid->Add(default_clrpkr_, {row,1}, {1,1});
    ++row;

    grid->Add(new wxStaticText(scrolled_, wxID_ANY, "Root"), {row,0}, {1,1});
    grid->Add(root_clrpkr_, {row,1}, {1,1});
    ++row;

    for (int i=0; i<PALETTE.size(); ++i) {
        wxColourPickerCtrl *pkr = new wxColourPickerCtrl(scrolled_, wxID_ANY);
        centroid_clrpkrs_.push_back(pkr);
        grid->Add(new wxStaticText(scrolled_, wxID_ANY, wxString::Format("Centroid %d", i+1)), {row,0}, {1,1});
        grid->Add(pkr, {row,1}, {1,1});
        ++row;
    }

    grid->SetHGap(3);

    scrolled_->SetSizer(grid);
    scrolled_->FitInside();
    scrolled_->SetScrollRate(5, 5);

    wxPanel *buttonPanel = new wxPanel(this);
    wxBoxSizer *buttonSizer = new wxBoxSizer(wxHORIZONTAL);
    wxButton *resetButton = new wxButton(buttonPanel, wxID_RESET, "Reset");
    buttonSizer->Add(resetButton, 1, wxEXPAND);
    buttonPanel->SetSizer(buttonSizer);

    wxBoxSizer *sizer = new wxBoxSizer(wxVERTICAL);
    sizer->Add(scrolled_, 1, wxEXPAND);
    sizer->Add(buttonPanel, 0, wxEXPAND);

    this->SetSizer(sizer);

    SetDefaults();

    default_clrpkr_->Bind(wxEVT_COLOURPICKER_CHANGED, &StyleEditor::OnColourPicked, this);
    root_clrpkr_->Bind(wxEVT_COLOURPICKER_CHANGED, &StyleEditor::OnColourPicked, this);

    for (wxColourPickerCtrl *pkr : centroid_clrpkrs_) {
        pkr->Bind(wxEVT_COLOURPICKER_CHANGED, &StyleEditor::OnColourPicked, this);
    }
    resetButton->Bind(wxEVT_BUTTON, &StyleEditor::OnResetClicked, this);
    Bind(wxEVT_CLOSE_WINDOW, &StyleEditor::OnCloseWindow, this);
}

wxColor
StyleEditor::GetColorForCentroid(int i) const {
    return centroid_clrpkrs_.at(i % centroid_clrpkrs_.size())->GetColour();
}

void
StyleEditor::OnColourPicked(wxColourPickerEvent &evt) {
    NotifyColorsChanged();
}

void
StyleEditor::OnResetClicked(wxCommandEvent &evt) { 
    SetDefaults();
    NotifyColorsChanged();
}

void
StyleEditor::SetDefaults() {
    default_clrpkr_->SetColour(*wxWHITE);
    root_clrpkr_->SetColour(*wxBLACK);
    for (size_t i=0; i<PALETTE.size(); ++i)
        centroid_clrpkrs_[i % centroid_clrpkrs_.size()]->SetColour(wxColor(PALETTE[i%PALETTE.size()]));
}
*/