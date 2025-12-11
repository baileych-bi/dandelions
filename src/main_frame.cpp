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

#include <array>
#include <filesystem>
#include <fstream>
#include <memory>
#include <numbers>
#include <numeric>

#include <wx/gbsizer.h>
#include <wx/numdlg.h>

#include "main_frame.h"
#include "network.h"
#include "muttable.h"
#include "parsers.h"
#include "tree.h"

namespace fs = std::filesystem;

MainFrame::MainFrame(const wxString &title, const wxPoint &pos, const wxSize &size)
    : wxFrame(NULL, wxID_ANY, title, pos, size) {

    style_editor_ = new StyleEditor(this);

    wxMenu *menuFile = new wxMenu;
    menuFile->Append(wxID_OPEN);
    menuFile->Append(ID_EXPORT_GRAPHIC, "Export Graphic");
    menuFile->Append(ID_EXPORT_TABLE, "Export Mutations");
    menuFile->Append(ID_EXPORT_SEQUENCES, "Export Centroid Sequences");
    menuFile->Append(ID_EXPORT_ADJACENCY, "Export Adjacency List");
    menuFile->Append(ID_EXPORT_MARKOV, "Export Markov Model");
    menuFile->AppendSeparator();
    menuFile->Append(wxID_EXIT);

    wxMenu *menuEdit = new wxMenu;
    menuEdit->Append(ID_EDIT_STYLE, "Style");

    wxMenu *menuHelp = new wxMenu;
    #ifdef WIN32
    menuHelp->Append(ID_HELP_CONSOLE, "Show Console");
    #endif
    menuHelp->Append(wxID_ABOUT);

    wxMenuBar *menuBar = new wxMenuBar;
    menuBar->Append(menuFile, "&File");
    menuBar->Append(menuEdit, "&Edit");
    menuBar->Append(menuHelp, "&Help");
    SetMenuBar(menuBar);
    CreateStatusBar();

    wxBoxSizer *sizer = new wxBoxSizer(wxHORIZONTAL);
    canvas_ = new Canvas(this);
    wxPanel *controlPanel = new wxPanel(this);
    wxGridBagSizer *controlPanelGrid = new wxGridBagSizer(1, 1);

    //sliders control the Constant objects in Network
    std::vector<std::pair<char, std::string>> slider_data = {
        {'G', "Repulsion"},
        {'K', "Tension"},
        {'E', "Scale"},
        {'B', "Drag"},
        {'C', "Compaction"},
        {'V', "Stability"},
        {'T', "Time"}
    };

    int row = 0;

    for (const auto &[C, label] : slider_data) {
        wxSlider *slider = new wxSlider(controlPanel, wxID_ANY, 0, 0, 10000);
        slider->SetMinSize(wxSize(100, slider->GetMinHeight()));
        controlPanelGrid->Add(new wxStaticText(controlPanel, wxID_ANY, label), {row,0}, {1,1});
        controlPanelGrid->Add(slider, {row,1}, {1,1});
        slider->Bind(wxEVT_SCROLL_CHANGED, &MainFrame::OnSliderScrollChanged, this);
        constant_sliders_[C] = slider;
        ++row;
    }

    auto_track_button_ = new wxToggleButton(controlPanel, wxID_ANY, "Auto Track");
    auto_track_button_->SetValue(true);
    controlPanelGrid->Add(auto_track_button_, {row,0}, {1,2}, wxEXPAND);
    ++row;

;    run_button_ = new wxToggleButton(controlPanel, wxID_ANY, "Run");
    controlPanelGrid->Add(run_button_, {row,0}, {1, 2}, wxEXPAND);
    ++row;

    controlPanel->SetSizer(controlPanelGrid);

    sizer->Add(canvas_, 1, wxEXPAND);
    sizer->Add(controlPanel, 0, wxEXPAND);

    this->SetSizer(sizer);
    SetAutoLayout(true);
    Layout();
    sizer->SetSizeHints(this);

    auto_track_button_->Bind(wxEVT_TOGGLEBUTTON, &MainFrame::OnAutoTrackButtonClicked, this);
    run_button_->Bind(wxEVT_TOGGLEBUTTON, &MainFrame::OnRunButtonClicked, this);

    Bind(AUTO_TRACK_CHANGED, &MainFrame::OnCanvasAutoTrackChanged, this);

    Bind(wxEVT_COMMAND_MENU_SELECTED, &MainFrame::OnOpen,            this, wxID_OPEN);
    Bind(wxEVT_COMMAND_MENU_SELECTED, &MainFrame::OnEditStyle,       this, ID_EDIT_STYLE);
    Bind(wxEVT_COMMAND_MENU_SELECTED, &MainFrame::OnExportGraphic,   this, ID_EXPORT_GRAPHIC);
    Bind(wxEVT_COMMAND_MENU_SELECTED, &MainFrame::OnExportTable,     this, ID_EXPORT_TABLE);
    Bind(wxEVT_COMMAND_MENU_SELECTED, &MainFrame::OnExportSequences, this, ID_EXPORT_SEQUENCES);
    Bind(wxEVT_COMMAND_MENU_SELECTED, &MainFrame::OnExportAdjacency, this, ID_EXPORT_ADJACENCY);
    Bind(wxEVT_COMMAND_MENU_SELECTED, &MainFrame::OnExportMarkov,    this, ID_EXPORT_MARKOV);
    Bind(wxEVT_COMMAND_MENU_SELECTED, &MainFrame::OnExit,            this, wxID_EXIT);
    Bind(wxEVT_COMMAND_MENU_SELECTED, &MainFrame::OnHelpConsole,     this, ID_HELP_CONSOLE);
    Bind(wxEVT_COMMAND_MENU_SELECTED, &MainFrame::OnAbout,           this, wxID_ABOUT);

    #ifdef _WIN32
    SetIcon(wxICON(appicon));
    #endif
}

void
MainFrame::SyncSliders(bool use_network_values) {
    std::shared_ptr<Network> net = canvas_->GetNetwork();
    if (!net) return;
    if (use_network_values) {
        for (auto &[C, slider] : constant_sliders_) {
            int value = slider->GetMin() + static_cast<int>(
                net->constant(C).as_fraction() * (slider->GetMax() - slider->GetMin())
                );
            slider->SetValue(value);
        }
    } else {
        for (auto &[C, slider] : constant_sliders_) {
            float value = (slider->GetValue() - slider->GetMin()) /
                static_cast<float>(slider->GetMax() - slider->GetMin());
            net->constant(C).set_fraction(value);
        }
    }
}

void
MainFrame::StylizeNodes(std::shared_ptr<Network> net) {
    if (!net || net->nodes().empty()) return;

    Node &root = net->node(0);

    //set Node area proportional to total represented sequences. z-height defaults to -1.
    //reset default label and such
    for (auto &[_, n] : *net) {
        n.r = sqrtf(std::max(1, n.total - n.inferred));
        n.style.set_defaults();
    }

    //any node that consists wholly of inferred seqeunces is labeled with a ?
    for (auto &[id, n] : *net) {
        if (n.total == n.inferred) n.style.label = wxString("?");
    }

    for (Node *c : net->centroids()) {
        c->style.label = wxString::Format("%d", c->centroid_id + 1);
        c->style.z = 2 * net->centroids().size() - c->centroid_id;
        for (Node *n = c->parent(); n->centroid_id == Node::NA && n->parent(); n = n->parent()) {
            n->style.z = net->centroids().size() - c->centroid_id;
        }
    }

    for (auto &[_, n] : *net) {
        if (n.is_root()) {
            n.style.brush.SetColour(style_editor_->GetRootColor());
        } else if (n.centroid_id < 0) {
            n.style.brush.SetColour(style_editor_->GetDefaultColor());
        } else {
            wxColor color = style_editor_->GetColorForCentroid(n.centroid_id);
            n.style.brush.SetColour(color);
            n.style.edge_pen.SetColour(color);
            n.style.edge_pen.SetWidth(2);
        }
    }

    for (auto rc = net->centroids().rbegin(); rc != net->centroids().rend(); ++rc) {
        Node *c = *rc;
        for (Node *n=c->parent(); n->centroid_id == Node::NA && n->parent(); n = n->parent()) {
            n->style.brush = c->style.brush;
            n->style.edge_pen = c->style.edge_pen;
        }
    }

    for (auto &[_, n] : *net) if (n.total == n.inferred) n.style.brush.SetColour(style_editor_->GetRootColor());

    root.style.z = 0;
}

void
MainFrame::LabelTopNCentroids(std::shared_ptr<Network> net, size_t top_n) {
    if (!net) return;

    std::vector<size_t> priority;
    for (auto &[id, n] : net->nodes()) if (!n.is_root()) priority.push_back(id);

    std::sort(priority.begin(),
              priority.end(),
              [&](size_t id_a, size_t id_b)->bool{
                const Node &a = net->node(id_a), &b = net->node(id_b);
                if (a.children().size() + a.total > b.children().size() + b.total) return true;
                if (b.children().size() + b.total > a.children().size() + a.total) return false;
                return false;
              });

    if (top_n < priority.size()) priority.resize(top_n);
    net->identify_centroids(priority);
    style_editor_->SetNumberCentroids(priority.size());
}

void
MainFrame::LabelAutoThresholdCentroids(std::shared_ptr<Network> net, size_t N) {
    if (!net || net->nodes().empty()) return;

    //hold the histogram of node sizes (where "size" is node.total + node.children().size())
    std::map<size_t, double> hist;

    //populate histogram
    size_t total = 0;
    for (const auto &[_, n] : net->nodes()) {
        if (!n.is_root()) { 
            size_t bucket = n.children().size() + n.total - 1;
            hist[bucket] += 1;
            total += 1.0;
        }
    }

    for (auto &[bucket, count] : hist) count /= total; //normalize histogram

    //normalized histogram looks like a gamma distribution pdf but we don't try to fit one;
    //just calculate mean and variance
    double mean = 0.0;
    double variance = 0.0;
    for (auto &[bucket, count] : hist) mean += bucket * count;
    for (auto &[bucket, count] : hist) variance += std::pow(bucket * count - mean, 2.0);

    const double X_THRESHOLD = mean + 6*std::sqrt(variance); //take 6-sigma as our centroid threshold
    std::vector<size_t> ids;
    for (const auto &[id, n] : net->nodes()) {
        if (n.children().size() + n.total - 1 >= X_THRESHOLD) ids.push_back(id);
    }
    net->identify_centroids(ids);
    style_editor_->SetNumberCentroids(ids.size());
}

void
MainFrame::OnRunButtonClicked(wxCommandEvent &event) {
    if (run_button_->GetLabel() == "Run") {
        run_button_->SetLabel("Pause");
        canvas_->StartAnimation();
    } else {
        canvas_->StopAnimation();
        run_button_->SetLabel("Run");
    }
}

void
MainFrame::OnAutoTrackButtonClicked(wxCommandEvent &evt) {
    canvas_->SetAutoTrack(auto_track_button_->GetValue());
}

void
MainFrame::OnCanvasAutoTrackChanged(wxCommandEvent &evt) {
    auto_track_button_->SetValue(canvas_->GetAutoTrack());
}

void
MainFrame::OnSliderScrollChanged(wxScrollEvent &evt) {
    std::shared_ptr<Network> net = canvas_->GetNetwork();
    if (!net) return;

    for (const auto &[C, slider] : constant_sliders_) {
        if (evt.GetEventObject() == slider) {
            float f = (static_cast<float>(slider->GetValue()) - slider->GetMin()) /
                (slider->GetMax() - slider->GetMin());
            net->constant(C).set_fraction(f);
            break;
        }
    }
}

void
MainFrame::OnStyleEdited(wxCommandEvent &evt) {
    std::shared_ptr<Network> net = canvas_->GetNetwork();
    StylizeNodes(net);
    canvas_->Refresh();
}

void
MainFrame::OnNumberCentroidsChanged(wxCommandEvent &evt) {
    std::shared_ptr<Network> net = canvas_->GetNetwork();
    if (net) {
        LabelTopNCentroids(net, style_editor_->GetNumberCentroids());
        StylizeNodes(net);
    }
    canvas_->Refresh();
}

size_t
count_diffs(std::string_view a, std::string_view b) {
    size_t diffs = 0;
    for (size_t i=0; i<a.size(); ++i) diffs += (a[i] != b[i]);
    return diffs;
}

/** Return offset, length for each gapped codon. */
std::vector<std::pair<int, int>>
find_codon_boundaries(std::string_view seq) {
    std::vector<std::pair<int, int>> result;
    int cnt = 0;
    int lo = 0;
    for (size_t i = 0; i != seq.size(); ++i) {
        if ('-' != seq[i]) {
            ++cnt;
            if (1 == cnt) {
                lo = i;
            } else if (3 == cnt) {
                result.push_back({lo, i - lo + 1});
                cnt = 0;
            }
        }
    }

    return result;
}

/** Calculate the intersection (overlap) of two intervals. Intervals are
* std::pair<int, int> where the .first member is the lower bound
* and the .second member is the _length_ NOT the upper bound.
* @param ivala the first interval
* @param ivalb the second interval
* @return an interval representing the overlap or std::nullopt if there is no overlap
*/
std::optional<std::pair<int, int>>
intersect(std::pair<int, int> ivala, std::pair<int, int> ivalb) {
    std::optional<std::pair<int, int>> result;
    int lo = std::max(ivala.first, ivalb.first);
    int hi = std::min(ivala.first + ivala.second, ivalb.first + ivalb.second);
    if (lo < hi) result = {lo, hi - lo};
    return result;
}

/** Perform Needleman-Wunsch alignment of amino acids using aligned nucleotide
* sequences as input. Essentially, an amino acid match is assigned a score
* of -inf if the nucleotides of two codons do not overlap in the MSA. For two
* ungapped sequences of the same length, the result reduces to the translatons
* of the top and bottom sequences with no gaps, consistent with Dandelions' 
* usual assumption of no indels. 
* @param seqa the top nucleotide sequence
* @param seqb the bottom nucleotide sequence
* @param gapp the gap penalty
* @return pair of strings where .first and .second are the aligned amino acid sequences
*/
std::pair<std::string, std::string>
contrained_nw_align(std::string_view seqa, std::string_view seqb, float gapp=4.0f) {
    std::vector<std::pair<int, int>> cdnsa = find_codon_boundaries(seqa);
    std::vector<std::pair<int, int>> cdnsb = find_codon_boundaries(seqb);

    //init the constrained match matrix
    Matrix<float> match(cdnsa.size(), cdnsb.size(), -std::numeric_limits<float>::infinity());
    for (size_t i = 0; i != cdnsa.size(); ++i)
    for (size_t j = 0; j != cdnsb.size(); ++j) {
        const std::optional<std::pair<int, int>> overlap = intersect(cdnsa[i], cdnsb[j]);
        if (overlap) {
            float score = 0;
            const auto &[lo, len] = *overlap;
            for (int k = lo; k != lo + len; ++k) {
                if (seqa[k] == seqb[k] && seqa[k] != '-') score += 1.0f;
            }
            match[{i, j}] = score;
        }
    }

    struct Trace {
        char mv = 0; //'m' = match, 'a' = insert gap in a, 'b' = insert gap in b
        float score = 0.0f;
    };

    Matrix<Trace> trace(cdnsa.size() + 1, cdnsb.size() + 1);
    for (size_t i = 1; i < trace.rows(); ++i) trace[{i, 0}].mv = 'b';
    for (size_t j = 1; j < trace.cols(); ++j) trace[{0, j}].mv = 'a';
    for (size_t i=1; i != cdnsa.size() + 1; ++i)
    for (size_t j=1; j != cdnsb.size() + 1; ++j) {
        std::array<Trace, 3> options = {
            Trace{.mv = 'm', .score = trace[{i - 1, j - 1}].score + match[{i - 1, j - 1}]},
            Trace{.mv = 'a', .score = trace[{i    , j - 1}].score - gapp * static_cast<bool>(cdnsb.size() != j)},
            Trace{.mv = 'b', .score = trace[{i - 1, j    }].score - gapp * static_cast<bool>(cdnsa.size() != i)}
        };

        trace[{i, j}] = *std::max_element(
            options.begin(),
            options.end(),
            [](const Trace &a, const Trace &b)->bool{ return a.score < b.score; });
    }

    std::string top;
    std::string btm;

    size_t i = trace.rows() - 1, j = trace.cols() - 1;
    Trace t = trace[{i, j}];
    for (;;) {
        if (t.mv == 'm') {
            const auto &[alo, alen] = cdnsa[i - 1];
            top.push_back(translate(std::string_view(&seqa[alo], alen))[0]);
            const auto &[blo, blen] = cdnsb[j - 1];
            btm.push_back(translate(std::string_view(&seqb[blo], blen))[0]);
            --i; --j;
        } else if (t.mv == 'a') {
            top.push_back('-');
            const auto &[blo, blen] = cdnsb[j - 1];
            btm.push_back(translate(std::string_view(&seqb[blo], blen))[0]);
            --j;
        } else if (t.mv == 'b') {
            const auto & [alo, alen] = cdnsa[i - 1];
            top.push_back(translate(std::string_view(&seqa[alo], alen))[0]);
            btm.push_back('-');
            --i;
        } else {
            break;
        }
        t = trace[{i,j}];
    }

    std::reverse(top.begin(), top.end());
    std::reverse(btm.begin(), btm.end());

    return std::make_pair<std::string>(std::move(top), std::move(btm));
}

/** Given two aligned amino acid sequences, top and btm, return a string
* describing the mutations, insertions, and deletions in btm relative to
* top. */
std::string
tally_alignment_mutations(std::string_view top, std::string_view btm) {
    assert(top.size() == btm.size());

    struct Mut {
        char type = 0;
        size_t pos = 0;
        std::string top;
        std::string btm;
    };
    std::vector<Mut> muts;

    for (size_t i = 0, j = 0; i != top.size(); ++i) {
        if (top[i] == '-' && btm[i] != '-') {
            muts.push_back(Mut{.type = '+', .pos=j, .btm=std::string(&btm[i], 1)});
        } else if (top[i] != '-' && btm[i] == '-') {
            muts.push_back(Mut{.type='-', .pos=j, .top=std::string(&top[i], 1)});
        } else if (top[i] != btm[i]) {
            muts.push_back(Mut{.pos=j, .top=std::string(&top[i], 1), .btm=std::string(&btm[i], 1)});
        }
        j += (top[i] != '-');
    }

    std::vector<Mut> consolidated;
    while (!muts.empty()) {
        if (muts.size() == 1) {
            consolidated.push_back(muts.back());
        } else {
            Mut &a = *(muts.rbegin() + 1);
            Mut &b = *(muts.rbegin() + 0);
            if (a.type && a.type == b.type && a.pos + 1 == b.pos) {
                a.top += b.top;
                a.btm += b.btm;
            } else {
                consolidated.push_back(muts.back());
            }
        }
        muts.pop_back();
    }

    std::reverse(consolidated.begin(), consolidated.end());

    std::string output;
    for (const Mut &m : consolidated) {
        if (m.type == '+') {
            output += '+' + std::to_string(m.pos + 1) + m.btm;
        } else if (m.type == '-') {
            output += '-' + std::to_string(m.pos + 1) + m.top;
        } else {
            output += m.top + std::to_string(m.pos + 1) + m.btm;
        }
        output.push_back(',');
    }
    if (!output.empty()) output.pop_back(); //remove trailing comma
    return output;
}

/** Called from File->Open. Open an input file, build, and display the tree. */
void
MainFrame::OnOpen(wxCommandEvent &evt) {
    bool first_load = !static_cast<bool>(canvas_->GetNetwork());

    wxFileDialog openDialog(
        this,
        "Open dsa output file or other list of nucleotide sequences",
        "",
        "",
        "dsa output files (*.csv)|*.csv|fasta files (*.fasta)|*.fasta|text files (*.txt)|*.txt",
        wxFD_OPEN | wxFD_FILE_MUST_EXIST);
    if (openDialog.ShowModal() == wxID_CANCEL) return;

    fs::path path = openDialog.GetPath().ToStdString();

    RunParametersDialog paramDialog(this, wxID_ANY, "Analysis Parameters");
    if (paramDialog.ShowModal() == wxID_CANCEL) return;

    bool run_inference = paramDialog.GetInferAncectors();
    int n_samples = paramDialog.GetNSamples();

    if (n_samples == -1) return;

    //parse a dsa file to get sequences and adjacency list for the consensus tree
    std::vector<std::string> sequences;
    try {
        std::ifstream ifs(path);
        if (!ifs) throw std::runtime_error("File " + path.string() + " could not be opened for reading.");
        decltype(&parse_dsa) methods[] = {parse_dsa, parse_fasta, parse_text};
        sequences = methods[openDialog.GetFilterIndex()](ifs);
        ifs.close();
    } catch (std::exception &e) {
        wxString msg;
        msg << "File " << path.filename().string() << " not found or invalid format.";
        wxMessageDialog errorDialog(
            this,
            msg,
            ""
        );
        errorDialog.SetExtendedMessage(e.what());
        errorDialog.ShowModal();
        return;
    }

    canvas_->StopAnimation();

    //ancestral inference doesn't work well with gaps so we warn the user if they checked "infer ancestors" and used an aligned input
    if (paramDialog.GetInferAncectors()) {
        for (const std::string &seq : sequences) {
            if (seq.find('-') != std::string::npos) {
                wxMessageDialog warning(this, "The input sequences contain gap characters. "
                "In general, ancestral inference does not work well with multiple sequence alignments, "
                "paarticularly if they contain frameshifts. Therefore, if indels make up a substantial "
                "portion of the data, consider re-analyzing the data with ancestral inference disabled "
                "for comparison.");
                warning.ShowModal();
                break;
            }
        }
    }

    std::vector<Edge> adj_list = build_consensus_mst(sequences, n_samples, paramDialog.GetInferAncectors());
    std::shared_ptr<Network> net(new Network);

    adj_list_ = adj_list;
    sequences_ = sequences;

    Node &root = net->add_node(0); //add root
    for (auto [p, c, d, w] : adj_list) net->add_node(c); //make a node for each child
    for (auto [p, c, d, w] : adj_list) net->add_edge(p, c, static_cast<float>(d), w); //make an edge to each child from its parent
    for (size_t i = 0; i < net->nodes().size(); ++i) net->node(i).nts(sequences[i]); //set the nt sequence for every node

    //each Node gets a label that shows the amino acid mutations from the wild type/ancestor sequenc
    const std::string &ancestor = net->node(0).aas();

    //set node tooltips
    for (size_t i=1; i < net->nodes().size(); ++i) {
        std::pair<std::string, std::string> aln = contrained_nw_align(net->node(0).nts(), net->node(i).nts());
        std::vector<std::string_view> top_lines = wrap(aln.first,  80);
        std::vector<std::string_view> btm_lines = wrap(aln.second, 80);

        std::string muts = std::string("Phenotype: ") + tally_alignment_mutations(aln.first, aln.second); //mutations relative to root
        std::vector<std::string_view> mut_lines = wrap(muts, 80, ',');

        for (auto line : mut_lines) {
            net->node(i).style.tooltip << std::string(line) << "\n";
        }

        net->node(i).style.tooltip
            << "Root Distance (nt): " << std::to_string(count_diffs(net->node(0).nts(), net->node(i).nts()))
            << "\nAncestor Distance (nt): " << std::to_string(count_diffs(net->node(i).nts(), net->node(i).parent()->nts()))
            << "\nConfidence: " << std::to_string(net->node(i).confidence) //fraction of samples containing this edge
            << "\nRoot Alignment:";

        std::string mismatch;
        for (size_t j=0; j < top_lines.size(); ++j) {
            mismatch.clear();
            mismatch.resize(top_lines[j].size(), '|');
            for (size_t k = 0; k != mismatch.size(); ++k) if (top_lines[j][k] != btm_lines[j][k]) mismatch[k] = ' ';
            net->node(i).style.tooltip << "\n" << std::string(top_lines[j]);
            net->node(i).style.tooltip << "\n" << mismatch;
            net->node(i).style.tooltip << "\n" << std::string(btm_lines[j]);
            if (j != top_lines.size() - 1) net->node(i).style.tooltip << "\n";
        }
    }

    //merge all connected subgraphs that share the same translation
    net->consolidate([](const Node *a, const Node *b)->bool {return a->aas() == b->aas();});

    LabelAutoThresholdCentroids(net, 2);

    StylizeNodes(net);

    net->init_simulation();
    net->pin_node(0);

    SetStatusText(path.filename().string());
    run_button_->SetLabel("Run");
    canvas_->SetNetwork(net);
    SyncSliders(first_load);
}

void
MainFrame::OnEditStyle(wxCommandEvent &evt) {
    style_editor_->Show();
    style_editor_->Bind(STYLE_UPDATED,            &MainFrame::OnStyleEdited,            this);
    style_editor_->Bind(NUMBER_CENTROIDS_CHANGED, &MainFrame::OnNumberCentroidsChanged, this);
}

void
MainFrame::OnExportGraphic(wxCommandEvent &evt) {
    wxFileDialog saveFileDialog(
        this,
        "Export figure",
        "",
        "",
        "SVG files (*.svg)|*.svg|PNG files (*.png)|*.png",
        wxFD_SAVE | wxFD_OVERWRITE_PROMPT);

    if (saveFileDialog.ShowModal() == wxID_CANCEL) return;

    fs::path path = saveFileDialog.GetPath().ToStdString();
    if (path.empty()) return;

    const int file_type = saveFileDialog.GetFilterIndex();

    if (file_type == 0) {
        wxSVGFileDC dc(path.generic_string(), 1200, 1200, 300);
        canvas_->PaintSVG(dc);
    } else {
        wxImage img(wxSize(2048, 2048));
        canvas_->PaintImage(img);
        img.SaveFile(path.string(), wxBITMAP_TYPE_PNG);
    }
}

void
MainFrame::OnExportMarkov(wxCommandEvent &evt) {
    std::shared_ptr<Network> net = canvas_->GetNetwork();
    if (!net) return;

    wxFileDialog saveFileDialog(
        this,
        "Export Markov model",
        "",
        "",
        ".csv files (*.csv)|*.csv",
        wxFD_SAVE | wxFD_OVERWRITE_PROMPT);

    if (saveFileDialog.ShowModal() == wxID_CANCEL) return;

    fs::path path = saveFileDialog.GetPath().ToStdString();
    if (path.empty()) return;

    std::ofstream ofs(path);
    if (!ofs) {
        wxString msg;
        msg << "File " << path.filename().string() << " could not be opened for writing.";
        wxMessageDialog errorDialog(
            this,
            msg,
            ""
        );
        errorDialog.ShowModal();
    }

    Matrix<double> m = infer_markov_model(sequences_, adj_list_);

    ofs << "  ";
    for (char c : std::string("ACGT")) 
        ofs << std::left << std::setw(std::numeric_limits<double>::max_digits10 + 3) << c;
    ofs << '\n';
    for (size_t i = 0; i < m.rows(); ++i) {
        ofs << std::left << std::setw(2) << "ACGT"[i];
        for (size_t j = 0; j < m.cols(); ++j) {
            ofs << std::fixed
                << std::setprecision(std::numeric_limits<double>::max_digits10)
                << m[{i,j}];
                if (j + 1 != m.cols()) ofs << ' ';
        }
        ofs << '\n';
    }
    ofs.close();
}

void
MainFrame::OnExportTable(wxCommandEvent &evt) {
    std::shared_ptr<Network> net = canvas_->GetNetwork();
    if (!net) return;

    wxFileDialog saveFileDialog(
        this,
        "Export mutation table",
        "",
        "",
        "HTML files (*.html)|*.html",
        wxFD_SAVE | wxFD_OVERWRITE_PROMPT);

    if (saveFileDialog.ShowModal() == wxID_CANCEL) return;

    fs::path path = saveFileDialog.GetPath().ToStdString();
    if (path.empty()) return;

    std::vector<Node *> centroids;
    for (auto &[_, n] : *(canvas_->GetNetwork())) {
        if (n.centroid_id != Node::NA) centroids.push_back(&n);
    }
    std::sort(centroids.begin(), centroids.end(),
        [](Node *a, Node *b)->bool { return a->centroid_id < b->centroid_id; });

    std::vector<std::string> seqs;
    std::vector<wxColor> colors;

    Node &root = net->node(0);
    seqs.push_back(root.aas());
    colors.push_back(root.style.brush.GetColour());

    for (Node *n : centroids) {
        seqs.push_back(n->aas());
        colors.push_back(n->style.brush.GetColour());
    }

    MutTable table(seqs);
    std::string html = table.to_html(colors);

    std::ofstream ofs(path);
    ofs << html;

    if (!ofs) {
        wxString msg;
        msg << "File " << path.filename().string() << " could not be opened for writing.";
        wxMessageDialog errorDialog(
            this,
            msg,
            ""
        );
        errorDialog.ShowModal();
    }
    ofs.close();
}

void
MainFrame::OnExportSequences(wxCommandEvent &evt) {
    std::shared_ptr<Network> net = canvas_->GetNetwork();
    if (!net) return;

    wxFileDialog saveFileDialog(
        this,
        "Export centroid sequences",
        "",
        "",
        "fasta files (*.fasta)|*.fasta",
        wxFD_SAVE | wxFD_OVERWRITE_PROMPT);

    if (saveFileDialog.ShowModal() == wxID_CANCEL) return;

    fs::path path = saveFileDialog.GetPath().ToStdString();
    if (path.empty()) return;

    std::vector<Node *> centroids;
    for (auto &[_, n] : *(canvas_->GetNetwork())) {
        if (n.centroid_id != Node::NA) centroids.push_back(&n);
    }
    std::sort(centroids.begin(), centroids.end(),
        [](Node *a, Node *b)->bool { return a->centroid_id < b->centroid_id; });

    std::ofstream ofs(path);
    for (Node *c : centroids) {
        ofs << ">Centroid_" << (c->centroid_id+1) << std::endl;
        for (const auto &line : wrap(c->aas(), 80)) ofs << line << std::endl;
    }

    if (!ofs) {
        wxString msg;
        msg << "File " << path.filename().string() << " could not be opened for writing.";
        wxMessageDialog errorDialog(
            this,
            msg,
            ""
        );
        errorDialog.ShowModal();
    }
    ofs.close();
}

void
MainFrame::OnExportAdjacency(wxCommandEvent &evt) {
    std::shared_ptr<Network> net = canvas_->GetNetwork();
    if (!net) return;

    wxFileDialog saveFileDialog(
        this,
        "Export adjacency list and translations",
        "",
        "",
        "text files (*.txt)|*.txt",
        wxFD_SAVE | wxFD_OVERWRITE_PROMPT);

    if (saveFileDialog.ShowModal() == wxID_CANCEL) return;

    fs::path path = saveFileDialog.GetPath().ToStdString();
    if (path.empty()) return;

    std::ofstream ofs(path);
    if (!ofs) {
        wxString msg;
        msg << "File " << path.filename().string() << " could not be opened for writing.";
        wxMessageDialog errorDialog(
            this,
            msg,
            ""
        );
        errorDialog.ShowModal();
    }

    for (const auto &[id, n] : *net) {
        if (0 == n.id()) continue;
        ofs << "(" << n.parent()->id() << ", " << n.id() << "; " << n.confidence << ")\n";
    }
    ofs << "//\n";
    for (const auto &[id, n] : *net) {
        ofs << ">" << id << "\n";
        ofs << n.aas() << "\n";
    }

    ofs.close();
}

void MainFrame::OnHelpConsole(wxCommandEvent &evt) {
    #if defined _WIN32
    AllocConsole();
    FILE *fDummy;
    freopen_s(&fDummy, "CONIN$", "r", stdin);
    freopen_s(&fDummy, "CONOUT$", "w", stderr);
    freopen_s(&fDummy, "CONOUT$", "w", stdout);
    std::cout.clear();
    std::cerr.clear();
    std::cin.clear();
    #endif
}

void
MainFrame::OnExit(wxCommandEvent &evt) {
    Close(true);
}

void
MainFrame::OnAbout(wxCommandEvent &evt) {
    wxMessageBox("Dandelions B cell lineage graphs and clustering.\n"
    "Latest version available from https://github.com/baileych-bi/dandelions.\n"
    "Built with wxWidgets (https://www.wxwidgets.org/).\n"
    "Always cluster BCR sequences responsibly!", "About Dandelions", wxOK | wxICON_INFORMATION);
}

RunParametersDialog::RunParametersDialog(wxWindow *parent, wxWindowID id, const wxString &title)
    : wxDialog(parent, id, title, wxDefaultPosition, wxDefaultSize, wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER) {

    inferCheckBox_ = new wxCheckBox(this, wxID_ANY, "");
    inferCheckBox_->SetValue(false);
    samplesSpinCtrl_ = new wxSpinCtrl(this, wxID_ANY, "1", wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, 1, 1001, 1);

    wxBoxSizer *vbox = new wxBoxSizer(wxVERTICAL);

    wxGridSizer *grid = new wxGridSizer(2);
    grid->Add(new wxStaticText(this, wxID_ANY, "Infer Ancestors"), 0, wxALL, 5);
    grid->Add(inferCheckBox_, 0, wxALL, 5);
    grid->Add(new wxStaticText(this, wxID_ANY, "Sample Size [1..1001]"), 0, wxALL, 5);
    grid->Add(samplesSpinCtrl_, 0, wxALL, 5);

    vbox->Add(grid, 0, wxEXPAND, 5);
    vbox->AddStretchSpacer();

    wxStdDialogButtonSizer *btns = CreateStdDialogButtonSizer(wxOK | wxCANCEL);
    vbox->Add(btns);

    SetSizer(vbox);
    Layout();
    vbox->SetSizeHints(this);
}

bool
RunParametersDialog::GetInferAncectors() const {
    return inferCheckBox_->GetValue();
}

int
RunParametersDialog::GetNSamples() const {
    return samplesSpinCtrl_->GetValue();
}
