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

#ifndef CCB_MAIN_FRAME_H_
#define CCB_MAIN_FRAME_H_

#include <wx/wx.h>
#include <wx/dcsvg.h>
#include <wx/dialog.h>
#include <wx/sizer.h>
#include <wx/spinctrl.h>
#include <wx/tglbtn.h>

#include "canvas.h"
#include "style_editor.h"
#include "tree.h"

/** Main application window that displays the tree and its controls.
* Displays a Run/Pause toggle button for the physical simulation and
* a series of sliders that control simulation parameters.<br/>
* Menus allow loading of dsa data files, and export of tree graphics and
* mutation tables.
*/
class MainFrame : public wxFrame {
public:
    /** wxFrame constructor */
    MainFrame(const wxString &title, const wxPoint &pos, const wxSize &size);

    /** Get a reference to the Canvas widget that holds and paints our Network.
    * the Canvas does most of the actual work of painting and handling user interaction.
    */
    Canvas &GetCanvas() { return *canvas_; }

    /** Set control sliders to reflect Network Constants or vice-versa.
    * @param use_network_values if true, the gui sliders will be set to
    * values of Network Constants (G, K, etc.) <br/>
    * if false, the Constants in the Network will be set based on the gui sliders
    */
    void SyncSliders(bool use_network_values);

private:
    //custom IDs for our export menu items
    enum CustomMenuIDs {
        ID_EXPORT_GRAPHIC = wxID_HIGHEST + 1,
        ID_EXPORT_TABLE,
        ID_EXPORT_SEQUENCES,
        ID_EXPORT_ADJACENCY,
        ID_EXPORT_MARKOV,
        ID_EDIT_STYLE,
        ID_HELP_CONSOLE
    };

    void StylizeNodes(std::shared_ptr<Network>);
    void LabelTopNCentroids(std::shared_ptr<Network>, size_t top_n);
    void LabelAutoThresholdCentroids(std::shared_ptr<Network>, size_t n_sdev);

    Canvas *canvas_ = nullptr;

    wxToggleButton *auto_track_button_ = nullptr;
    wxToggleButton *run_button_        = nullptr;

    StyleEditor *style_editor_ = nullptr; 

    std::unordered_map<char, wxSlider *> constant_sliders_;

    std::vector<Edge> adj_list_;
    std::vector<std::string> sequences_;

    /** Open a dsa output file. */
    void OnOpen(wxCommandEvent &evt);

    /** Open the style editor. */
    void OnEditStyle(wxCommandEvent &evt);

    /** Export our Network drawing as SVG of .png */
    void OnExportGraphic(wxCommandEvent &evt);

    /** Export Markov model of nucleotide substitution */
    void OnExportMarkov(wxCommandEvent &evt);

    /** Export a HTML file with a mutation table. */
    void OnExportTable(wxCommandEvent &evt);

    /** Export a .fasta file with the amino acid sequences of the centroids. */
    void OnExportSequences(wxCommandEvent &evt);

    /** Export an adjacency list and list of sequences. */
    void OnExportAdjacency(wxCommandEvent &evt);

    /** For Windows only, call AllocConsole and attach cout, cin, etc. */
    void OnHelpConsole(wxCommandEvent &evt);

    /** Show About message */
    void OnAbout(wxCommandEvent &evt);

    void OnExit(wxCommandEvent &evt);
    void OnRunButtonClicked(wxCommandEvent &evt);
    void OnAutoTrackButtonClicked(wxCommandEvent &evt);
    void OnCanvasAutoTrackChanged(wxCommandEvent &evt);
    void OnSliderScrollChanged(wxScrollEvent &evt);
    void OnStyleEdited(wxCommandEvent &evt);
    void OnNumberCentroidsChanged(wxCommandEvent &evt);
};

/** Dialog that collects the parameters for MST consturction.
* Sets the following options:<br>
* Infer Ancestors: if set to true, neighbor joining and phylogentic inference
* will be performed for each sample of the MST forest<br/>
* Samples: the number of MSTs created from randomly permuted data used to form
* the consensus tree.<br/>
* Label Method: Top N will classify the N "largest" nodes as centroids (where
* node size is #non-coding variants + # of direct ancestors). Auto Threshold
* fits and exponential distribution (i.e., y = lambda * e^-(lambda*x)) to the
* histogram of node sizes and bisects the histogram at the point where the
* slope of the exponential fit is 1/(N+1)th of its initial steepness. Nodes
* to the right of this point are classified as centroids.<br/>
* N: The N for Top N or Auto Threshold methods.
*/
class RunParametersDialog : public wxDialog {
public:
    RunParametersDialog(wxWindow *parent, wxWindowID id, const wxString &title);

    enum LabelMethod {
        LABEL_METHOD_TOP_N = 0,
        LABEL_METHOD_AUTO  = 1
    };

    bool GetInferAncectors() const;
    int GetNSamples() const;

private:
    const int DEFAULT_TOP_N = 10;
    const int DEFAULT_THRESHOLD = 1;

    wxCheckBox *inferCheckBox_     = nullptr;
    wxSpinCtrl *samplesSpinCtrl_         = nullptr;
};

#endif