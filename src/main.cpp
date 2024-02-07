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

#include <fstream>
#include <memory>
#include <string>
#include <vector>

#include <wx/wx.h>

#include "main_frame.h"

/** Build and display a consensus minimum spanning tree (MST) that 
* (hopefully) reflects phylogeny and clustering of a set of related
* nucleotide sequences.<br/>
* Instructions for use:<br/>
* From the main window, File->Open one of the following:
* <ol>
* <li>a .dsa output file (run with --template_dna=... and --show_codons=horizontal options)</li>
* <li>or a .fasta file of nucleotide sequences</li>
* <li>or a or plain .txt file with a nucleotide sequence on each line</li>
* </ol>
* IMPORTANT: when opening dsa output, the tree will be rooted on the --template_dna
* sequence. For a .fasta or .txt file, the FIRST sequence encountered in the file will be used
* as the root. The program will remove duplicate sequences and any sequence whose length
* differs from that of the root (i.e., it does not yet handle indels).<br/>
* When prompted, enter the number of MSTs on which to base the consensus. The consensus algorith
* works such that for a sample size of 1 (the default) the 'consensus' is the same as the single
* input tree. Details of the consensus building algorith are give at the end.</br>
* The nucleotide consensus tree is then transformed into a tree of amino acid sequencs by merging
* all nodes in each subtree of the tree that share the same translation. The size of the nodes
* when displayed is proportional to the number of nodes merged in this process. Nodes with
* unusually high numbers (>2 s.d. above the mean) of direct descendants are designated as 
* 'centroids' and given special treatment. Each centroid is assigned a color and a number
* starting from 1. The lower the number, the more descendants a given centroid has. Centroids
* and the nodes and edges that connect them to the root node adopt the color of their centroid.
* The root node is painted black.<br/>
* The Nodes and edges are displayed using an interactive physics simulation similar to that
* of the vis.js library. Nodes are modeled as point masses that repel each other. Edges are
* modeled as springs. Edge length is proportional to the sequence distance between two nodes.<br/>
* The following parameters goven the simulation:
* <ol>
* <li>repulsion - 1/r^2 repulsive force between Nodes</li>
* <li>tension - spring constant</li>
* <li>scale - scale factor applied to the length of each edge/spring</li>
* <li>drag - drag coefficient</li>
* <li>compaction - global force field that pushes nodes toward the origin</li>
* <li>stability - limit to the maximum node velocity</li>
* <li>time - length of the timestep for each iteration</li>
* </ol>
* The graph can be exported to .svg or .png format using File->Export->Graphic. 
* File->Export->Mutations produces a formatted and color-coded table of sequence differences
* between each centroid and the root in a .html file.<br/> 
* The MST and consensus building algorithms:<br/>
* The individual MSTs that make up the consensus tree are built by first randomly shuffling the
* input sequences and following Prim's algorithm with the following modification: distance between
* nodes p (aleady in tree) and c (not yet in tree) is a 2-tuple of (D(c,p), D(p,r)) where r is
* the root node and D(i,j) is the Hamming Distance between the two seqeunces. This change generates
* trees that reflect a breadth-first approach search of the underlying biological system (as we expect
* for affinity maturation).<br/>
* To build the conensus tree, we generate n MSTs, each from a random permutation of the input sequences.
* Optionally, phylogenetic inference is performed on the input sequences with each iteration and the
* inferred ancestors are incorporated during MST construction, then removed from the final tree.
* We keep track of the frequency with which every node (except root) c is the direct descendant of 
* every other node p. This results in a (k-1)x(k) element distance matrix (where k is the number of 
* nodes and root has no ancestor) with elements M(c,p) in the range [0, 1]. We then construct a single 
* MST from M with edge distances of 1-M[c,p]. The algorithm has the nice property that for a sample size 
* of n=1, the output consensus tree is identical to its single n=1 input tree.
*/

class TheApp : public wxApp {
public:
    virtual bool OnInit();
    virtual bool OnExceptionInMainLoop() { throw; return false; }
    virtual void OnUnhandledException() { throw; }
};

IMPLEMENT_APP(TheApp);

bool 
TheApp::OnInit() {
    if (!wxApp::OnInit()) return false;

    #if defined _WIN32 && !defined (NDEBUG)
    AllocConsole();
    FILE *fDummy;
    freopen_s(&fDummy, "CONIN$",  "r", stdin );
    freopen_s(&fDummy, "CONOUT$", "w", stderr);
    freopen_s(&fDummy, "CONOUT$", "w", stdout);
    std::cout.clear();
    std::cerr.clear();
    std::cin.clear();
    #endif

    wxImage::AddHandler(new wxPNGHandler);

    MainFrame *frame = new MainFrame("Dandelions", wxPoint(50, 50), wxSize(450, 340));
    frame->Show(true);
    return true;
}
