#include "ultimaille/io/vtk.h"

#include <vtkDataSetReader.h>
#include <vtkDataSetWriter.h>
#include <vtkSmartPointer.h>
#include <vtkUnstructuredGrid.h>

#include <iostream>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <array>

#define FOR(i, n) for(int i = 0; i < static_cast<int>(n); i++)

namespace UM {
    void read_vtk_format(const std::string& filename, std::vector<vec3>& verts_, std::vector<int>& edges_, std::vector<int>& tris_, std::vector<int>& quads_, std::vector<int>& tets_, std::vector<int>& hexes_) {
        auto reader = vtkSmartPointer<vtkDataSetReader>::New();
        reader->SetFileName(filename.c_str());
        reader->Update();
        auto grid = vtkSmartPointer<vtkUnstructuredGrid>::New();
        grid->ShallowCopy(reader->GetUnstructuredGridOutput());

        verts_.resize(grid->GetNumberOfPoints());
        for (vtkIdType i = 0; i < grid->GetNumberOfPoints(); ++i) {
            double point[3];
            grid->GetPoint(i, point);
            verts_[i] = vec3{ point[0], point[1], point[2] };
        }

        auto ids = vtkSmartPointer<vtkIdList>::New();
        for (vtkIdType i = 0; i < grid->GetNumberOfCells(); ++i) {
            grid->GetCellPoints(i, ids);
            int cellType = grid->GetCellType(i);
            switch (cellType) {
                case VTK_LINE: // Edge
                    if (ids->GetNumberOfIds() == 2) {
                        edges_.push_back(ids->GetId(0));
                        edges_.push_back(ids->GetId(1));
                    }
                    break;
                case VTK_TRIANGLE: // Triangle
                    if (ids->GetNumberOfIds() == 3) {
                        tris_.push_back(ids->GetId(0));
                        tris_.push_back(ids->GetId(1));
                        tris_.push_back(ids->GetId(2));
                    }
                    break;
                case VTK_QUAD: // Quad
                    if (ids->GetNumberOfIds() == 4) {
                        quads_.push_back(ids->GetId(0));
                        quads_.push_back(ids->GetId(1));
                        quads_.push_back(ids->GetId(2));
                        quads_.push_back(ids->GetId(3));
                    }
                    break;
                case VTK_TETRA: // Tetrahedron
                    if (ids->GetNumberOfIds() == 4) {
                        tets_.push_back(ids->GetId(0));
                        tets_.push_back(ids->GetId(1));
                        tets_.push_back(ids->GetId(3)); // flip to to match convention
                        tets_.push_back(ids->GetId(2)); // flip to to match convention
                    }
                    break;
                case VTK_HEXAHEDRON: // Hexahedron
                    if (ids->GetNumberOfIds() == 8) {
                        hexes_.push_back(ids->GetId(0));
                        hexes_.push_back(ids->GetId(1));
                        hexes_.push_back(ids->GetId(2));
                        hexes_.push_back(ids->GetId(3));
                        hexes_.push_back(ids->GetId(4));
                        hexes_.push_back(ids->GetId(5));
                        hexes_.push_back(ids->GetId(6));
                        hexes_.push_back(ids->GetId(7));
                    }
                    break;
                default:
                    std::cerr << "Unsupported cell type: " << cellType << " in file: " << filename << std::endl;
                    break;
            }
        }

        std::cout << "VTK file read successfully: " << filename << std::endl;
        std::cout << "Vertices: " << verts_.size() << std::endl;
        std::cout << "Tetra: " << tets_.size() / 4 << std::endl;
    }


    void write_vtk_format(const std::string& filename, const std::vector<vec3>& verts_, const std::vector<int>& edges_, const std::vector<int>& tris_, const std::vector<int>& quads_, const std::vector<int>& tets_, const  std::vector<int>& hexes_) {
        auto grid = vtkSmartPointer<vtkUnstructuredGrid>::New();
        auto points = vtkSmartPointer<vtkPoints>::New();
        points->SetNumberOfPoints(verts_.size());
        for (size_t i = 0; i < verts_.size(); ++i) {
            points->SetPoint(i, verts_[i][0], verts_[i][1], verts_[i][2]);
        }
        grid->SetPoints(points);

        auto cells = vtkSmartPointer<vtkCellArray>::New();
        for (size_t i = 0; i < tets_.size() / 4; ++i) {
            const auto tet = &tets_[4*i];
            const vtkIdType ids[4] = { 
                tet[0], 
                tet[1], 
                tet[3], // flip to standard VTK order
                tet[2]  // flip to standard VTK order
            };
            cells->InsertNextCell(4, ids);
        }
        // WARNING: This assumes all cells are tetrahedra; you may need to handle other cell types similarly.
        grid->SetCells(VTK_TETRA, cells);

        auto writer = vtkSmartPointer<vtkDataSetWriter>::New();
        writer->SetFileName(filename.c_str());
        writer->SetInputData(grid);
        writer->SetFileTypeToBinary();
        writer->Write();
    }



    void write_vtk(const std::string filename, const PolyLine& pl) {
        std::vector<vec3> verts(pl.nverts());
        std::vector<int> edges(2 * pl.nsegments());
        std::vector<int> tris;
        std::vector<int> quads;
        std::vector<int> tets;
        std::vector<int> hexes;
        FOR(v, pl.nverts()) verts[v] = pl.points[v];
        FOR(e, pl.nsegments()) FOR(ev, 2) edges[2 * e + ev] = pl.vert(e, ev);
        write_vtk_format(filename, verts, edges, tris, quads, tets, hexes);
    }
    void write_vtk(const std::string filename, const Surface& m) {
        std::vector<vec3> verts(m.nverts());
        std::vector<int> edges;
        std::vector<int> tris;
        std::vector<int> quads;
        std::vector<int> tets;
        std::vector<int> hexes;
        FOR(v, m.nverts()) verts[v] = m.points[v];
        FOR(f, m.nfacets()) {
            if (m.facet_size(f) == 3) {
                FOR(i, 3) tris.push_back(m.vert(f, i));
            }
            else if (m.facet_size(f) == 4) {
                FOR(i, 4) quads.push_back(m.vert(f, i));
            }
            else {
                std::cerr << "Polygon are not supported in our MEDIT writer";
            }
        }
        write_vtk_format(filename, verts, edges, tris, quads, tets, hexes);
    }
    void write_vtk(const std::string filename, const Volume& m) {
        std::vector<vec3> verts(m.nverts());;
        std::vector<int> edges;
        std::vector<int> tris;
        std::vector<int> quads;
        std::vector<int> tets;
        std::vector<int> hexes;
        FOR(v, m.nverts()) verts[v] = m.points[v];
        if (m.cell_type() == 0) {
            tets.resize(4 * m.ncells());
            FOR(t, m.ncells()) FOR(tv, 4) tets[4 * t + tv] = m.vert(t, tv);
        }
        else if (m.cell_type() == 1) {
            hexes.resize(8 * m.ncells());
            FOR(h, m.ncells()) FOR(hv, 8) hexes[8 * h + hv] = m.vert(h, hv);
        }
        else {
            std::cerr << "Volume type : " << m.cell_type() << "; not supported in our MEDIT writer";
        }

        write_vtk_format(filename, verts, edges, tris, quads, tets, hexes);
    }


    PolyLineAttributes read_vtk(const std::string filename, PolyLine& m) {
        std::vector<vec3> verts;
        std::vector<int> edges;
        std::vector<int> tris;
        std::vector<int> quads;
        std::vector<int> tets;
        std::vector<int> hexes;
        read_vtk_format(filename, verts, edges, tris, quads, tets, hexes);
        m = PolyLine();
        m.points.create_points(verts.size());
        FOR(v, verts.size()) m.points[v] = verts[v];
        m.create_segments(edges.size()/2);
        FOR(e, m.nsegments()) FOR(ev, 2) m.vert(e, ev) = edges[2 * e + ev];
        return {};
    }

    SurfaceAttributes read_vtk(const std::string filename, Triangles& m) {
        std::vector<vec3> verts;
        std::vector<int> edges;
        std::vector<int> tris;
        std::vector<int> quads;
        std::vector<int> tets;
        std::vector<int> hexes;
        read_vtk_format(filename, verts, edges, tris, quads, tets, hexes);
        m = Triangles();
        m.points.create_points(verts.size());
        FOR(v, verts.size()) m.points[v] = verts[v];
        m.create_facets(tris.size() / 3);
        FOR(t, m.nfacets()) FOR(tv, 3) m.vert(t, tv) = tris[3 * t + tv];
        return {};
    }

    SurfaceAttributes read_vtk(const std::string filename, Quads& m) {
        std::vector<vec3> verts;
        std::vector<int> edges;
        std::vector<int> tris;
        std::vector<int> quads;
        std::vector<int> tets;
        std::vector<int> hexes;
        read_vtk_format(filename, verts, edges, tris, quads, tets, hexes);
        m = Quads();
        m.points.create_points(verts.size());
        FOR(v, verts.size()) m.points[v] = verts[v];
        m.create_facets(quads.size() / 4);
        FOR(q, m.nfacets()) FOR(qv, 4) m.vert(q, qv) = quads[4 * q + qv];
        return {};
    }

    SurfaceAttributes read_vtk(const std::string filename, Polygons& m) {
        std::vector<vec3> verts;
        std::vector<int> edges;
        std::vector<int> tris;
        std::vector<int> quads;
        std::vector<int> tets;
        std::vector<int> hexes;
        read_vtk_format(filename, verts, edges, tris, quads, tets, hexes);
        m = Polygons();
        m.points.create_points(verts.size());
        FOR(v, verts.size()) m.points[v] = verts[v];

        m.create_facets(tris.size() / 3, 3);
        FOR(t, m.nfacets()) FOR(tv, 3) m.vert(t, tv) = tris[3 * t + tv];

        int off = m.create_facets(quads.size() / 4, 4);
        FOR(q, quads.size() / 4) FOR(qv, 4) m.vert(off + q, qv) = quads[4 * q + qv];
        return {};
    }

    VolumeAttributes read_vtk(const std::string filename, Tetrahedra& m) {
        std::vector<vec3> verts;
        std::vector<int> edges;
        std::vector<int> tris;
        std::vector<int> quads;
        std::vector<int> tets;
        std::vector<int> hexes;
        read_vtk_format(filename, verts, edges, tris, quads, tets, hexes);
        m = Tetrahedra();
        m.points.create_points(verts.size());
        FOR(v, verts.size()) m.points[v] = verts[v];
        m.create_cells(tets.size() / 4);
        FOR(t, m.ncells()) FOR(tv, 4) m.vert(t, tv) = tets[4 * t + tv];
        return {};
    }

    VolumeAttributes read_vtk(const std::string filename, Hexahedra& m) {
        std::vector<vec3> verts;
        std::vector<int> edges;
        std::vector<int> tris;
        std::vector<int> quads;
        std::vector<int> tets;
        std::vector<int> hexes;
        read_vtk_format(filename, verts, edges, tris, quads, tets, hexes);
        m = Hexahedra();
        m.points.create_points(verts.size());
        FOR(v, verts.size()) m.points[v] = verts[v];

        m.create_cells(tets.size() / 8);
        FOR(h, m.ncells()) FOR(hv, 8) m.vert(h, hv) = hexes[8 * h + hv];
        return{};
    }

}

