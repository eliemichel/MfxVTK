/*
MfxVTK Open Mesh Effect plug-in
Copyright (c) 2020 Tomas Karabela

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.
*/

#include <vtkFloatArray.h>
#include <vtkCellIterator.h>
#include <vtkPointData.h>
#include <vtkPolyData.h>
#include <vtkMinimalStandardRandomSequence.h>
#include <vtkSmartPointer.h>
#include <vtkSmoothPolyDataFilter.h>
#include <vtkWindowedSincPolyDataFilter.h>
#include <vtkPolyDataPointSampler.h>
#include <vtkFeatureEdges.h>
#include <vtkFillHolesFilter.h>
#include <vtkExtractEdges.h>
#include <vtkTubeFilter.h>
#include <vtkCleanPolyData.h>
#include <vtkTriangleFilter.h>
#include <vtkDecimatePro.h>
#include <vtkQuadricDecimation.h>
#include <vtkQuadricClustering.h>
#include <vtkAppendPolyData.h>
#include <vtkMaskPoints.h>
#include <vtkDelaunay3D.h>
#include <vtkGeometryFilter.h>
#include <vtkImplicitPolyDataDistance.h>
#include <vtkStaticCellLinks.h>
#include <vtkExtractCells.h>

#include "ofxCore.h"
#include "ofxMeshEffect.h"

#include "PluginSupport/MfxRegister"
#include "VtkEffect.h"
#include "utils.h"

bool is_positive_double(double x) {
    return x >= DBL_EPSILON;
}

class VtkSmoothPolyDataFilterEffect : public VtkEffect {
private:
    const char *PARAM_ITERATIONS = "NumberOfIterations";
    const char *PARAM_CONVERGENCE = "Convergence";
    const char *PARAM_RELAXATION_FACTOR = "RelaxationFactor";
    const char *PARAM_BOUNDARY_SMOOTHING = "BoundarySmoothing";
    const char *PARAM_FEATURE_EDGE_SMOOTHING = "FeatureEdgeSmoothing";
    const char *PARAM_FEATURE_ANGLE = "FeatureAngle";
    const char *PARAM_EDGE_ANGLE = "EdgeAngle";

public:
    const char* GetName() override {
        return "Smooth (Laplacian)";
    }

    OfxStatus vtkDescribe(OfxParamSetHandle parameters) override {
        AddParam(PARAM_ITERATIONS, 20).Range(1, 1000).Label("Iterations");
        AddParam(PARAM_RELAXATION_FACTOR, 0.1).Range(0.0, 1000.0).Label("Relaxation factor");
        AddParam(PARAM_BOUNDARY_SMOOTHING, true).Label("Boundary smoothing");
        AddParam(PARAM_FEATURE_EDGE_SMOOTHING, false).Label("Feature edge smoothing");
        AddParam(PARAM_FEATURE_ANGLE, 45.0).Range(0.001, 180.0).Label("Feature angle");
        AddParam(PARAM_EDGE_ANGLE, 15.0).Range(0.001, 180.0).Label("Edge angle");
        AddParam(PARAM_CONVERGENCE, 0.0).Range(0.0, 1000.0).Label("Convergence");
        return kOfxStatOK;
    }

    OfxStatus vtkCook(vtkPolyData *input_polydata, vtkPolyData *output_polydata) override {
        int iterations = GetParam<int>(PARAM_ITERATIONS).GetValue();
        double convergence = GetParam<double>(PARAM_CONVERGENCE).GetValue();
        bool boundary_smoothing = GetParam<bool>(PARAM_BOUNDARY_SMOOTHING).GetValue();
        double relaxation_factor = GetParam<double>(PARAM_RELAXATION_FACTOR).GetValue();
        bool feature_edge_smoothing = GetParam<bool>(PARAM_FEATURE_EDGE_SMOOTHING).GetValue();
        double feature_angle = GetParam<double>(PARAM_FEATURE_ANGLE).GetValue();
        double edge_angle = GetParam<double>(PARAM_EDGE_ANGLE).GetValue();

        auto filter = vtkSmartPointer<vtkSmoothPolyDataFilter>::New();
        filter->SetInputData(input_polydata);

        filter->SetNumberOfIterations(iterations);
        filter->SetConvergence(convergence);
        filter->SetBoundarySmoothing(boundary_smoothing);
        filter->SetRelaxationFactor(relaxation_factor);
        filter->SetFeatureEdgeSmoothing(feature_edge_smoothing);
        filter->SetFeatureAngle(feature_angle);
        filter->SetEdgeAngle(edge_angle);

        filter->Update();

        auto filter_output = filter->GetOutput();
        output_polydata->ShallowCopy(filter_output);
        return kOfxStatOK;
    }
};

// ----------------------------------------------------------------------------

class VtkWindowedSincPolyDataFilterEffect : public VtkEffect {
private:
    const char *PARAM_ITERATIONS = "NumberOfIterations";
    const char *PARAM_PASSBAND = "PassBand";
    const char *PARAM_BOUNDARY_SMOOTHING = "BoundarySmoothing";
    const char *PARAM_NONMANIFOLD_SMOOTHING = "NonManifoldSmoothing";
    const char *PARAM_FEATURE_EDGE_SMOOTHING = "FeatureEdgeSmoothing";
    const char *PARAM_FEATURE_ANGLE = "FeatureAngle";
    const char *PARAM_EDGE_ANGLE = "EdgeAngle";
    const char *PARAM_NORMALIZE_COORDINATES = "NormalizeCoordinates";

public:
    const char* GetName() override {
        return "Smooth (windowed sinc)";
    }

    OfxStatus vtkDescribe(OfxParamSetHandle parameters) override {
        AddParam(PARAM_ITERATIONS, 20).Range(1, 1000).Label("Iterations");
        AddParam(PARAM_PASSBAND, 0.1).Range(0.001, 2.0).Label("Passband");
        AddParam(PARAM_BOUNDARY_SMOOTHING, true).Label("Boundary smoothing");
        AddParam(PARAM_NONMANIFOLD_SMOOTHING, false).Label("Non-manifold smoothing");
        AddParam(PARAM_FEATURE_EDGE_SMOOTHING, false).Label("Feature edge smoothing");
        AddParam(PARAM_FEATURE_ANGLE, 45.0).Range(0.001, 180.0).Label("Feature angle");
        AddParam(PARAM_EDGE_ANGLE, 15.0).Range(0.001, 180.0).Label("Edge angle");
        AddParam(PARAM_NORMALIZE_COORDINATES, true).Label("Normalize coordinates");
        return kOfxStatOK;
    }

    OfxStatus vtkCook(vtkPolyData *input_polydata, vtkPolyData *output_polydata) override {
        int iterations = GetParam<int>(PARAM_ITERATIONS).GetValue();
        double passband = GetParam<double>(PARAM_PASSBAND).GetValue();
        bool boundary_smoothing = GetParam<bool>(PARAM_BOUNDARY_SMOOTHING).GetValue();
        bool nonmanifold_smoothing = GetParam<bool>(PARAM_NONMANIFOLD_SMOOTHING).GetValue();
        bool feature_edge_smoothing = GetParam<bool>(PARAM_FEATURE_EDGE_SMOOTHING).GetValue();
        double feature_angle = GetParam<double>(PARAM_FEATURE_ANGLE).GetValue();
        double edge_angle = GetParam<double>(PARAM_EDGE_ANGLE).GetValue();
        bool normalize_coordinates = GetParam<bool>(PARAM_NORMALIZE_COORDINATES).GetValue();

        auto filter = vtkSmartPointer<vtkWindowedSincPolyDataFilter>::New();
        filter->SetInputData(input_polydata);

        filter->SetNumberOfIterations(iterations);
        filter->SetPassBand(passband);
        filter->SetBoundarySmoothing(boundary_smoothing);
        filter->SetNonManifoldSmoothing(nonmanifold_smoothing);
        filter->SetFeatureEdgeSmoothing(feature_edge_smoothing);
        filter->SetFeatureAngle(feature_angle);
        filter->SetEdgeAngle(edge_angle);
        filter->SetNormalizeCoordinates(normalize_coordinates);

        filter->Update();

        auto filter_output = filter->GetOutput();
        output_polydata->ShallowCopy(filter_output);
        return kOfxStatOK;
    }
};

// ----------------------------------------------------------------------------

class VtkPolyDataPointSamplerEffect : public VtkEffect {
private:
    const char *PARAM_DISTANCE = "Distance";
    // const char *PARAM_USE_RANDOM_GENERATION_MODE = "UseRandomGenerationMode";
    // const char *PARAM_GENERATE_VERTEX_POINTS = "GenerateVertexPoints";
    const char *PARAM_GENERATE_EDGE_POINTS = "GenerateEdgePoints";
    const char *PARAM_GENERATE_INTERIOR_POINTS = "GenerateInteriorPoints";
    const char *PARAM_INTERPOLATE_POINT_DATA = "InterpolatePointData";

public:
    const char* GetName() override {
        return "Point sampling";
    }

    OfxStatus vtkDescribe(OfxParamSetHandle parameters) override {
        AddParam(PARAM_DISTANCE, 0.1).Range(1e-6, 1e6).Label("Distance");
        // AddParam(PARAM_USE_RANDOM_GENERATION_MODE, true).Label("Random point generation");
        // AddParam(PARAM_GENERATE_VERTEX_POINTS, true).Label("Generate vertex points");
        AddParam(PARAM_GENERATE_EDGE_POINTS, true).Label("Generate edge points");
        AddParam(PARAM_GENERATE_INTERIOR_POINTS, true).Label("Generate interior points");
        AddParam(PARAM_INTERPOLATE_POINT_DATA, false).Label("Interpolate point data");
        return kOfxStatOK;
    }

    OfxStatus vtkCook(vtkPolyData *input_polydata, vtkPolyData *output_polydata) override {
        double distance = GetParam<double>(PARAM_DISTANCE).GetValue();
        // bool user_random_generation_mode = GetParam<bool>(PARAM_USE_RANDOM_GENERATION_MODE).GetValue();
        bool generate_vertex_points = true; // GetParam<bool>(PARAM_GENERATE_VERTEX_POINTS).GetValue(); // false crashes VTK 9.0.1, why?
        bool generate_edge_points = GetParam<bool>(PARAM_GENERATE_EDGE_POINTS).GetValue();
        bool generate_interior_points = GetParam<bool>(PARAM_GENERATE_INTERIOR_POINTS).GetValue();
        bool interpolate_point_data = GetParam<bool>(PARAM_INTERPOLATE_POINT_DATA).GetValue();

        auto append_poly_data = vtkSmartPointer<vtkAppendPolyData>::New();

        auto vertex_edge_sampler = vtkSmartPointer<vtkPolyDataPointSampler>::New();
        vertex_edge_sampler->SetInputData(input_polydata);
        vertex_edge_sampler->SetDistance(distance);
        vertex_edge_sampler->SetGenerateVertexPoints(generate_vertex_points);
        vertex_edge_sampler->SetGenerateEdgePoints(generate_edge_points);
        vertex_edge_sampler->SetGenerateInteriorPoints(false);
        vertex_edge_sampler->SetInterpolatePointData(interpolate_point_data);
        vertex_edge_sampler->Update();

        append_poly_data->AddInputData(vertex_edge_sampler->GetOutput());

        if (generate_interior_points) {
            // to handle non-convex polygons correctly, we need to triangulate first; fixes #2
            auto triangle_filter = vtkSmartPointer<vtkTriangleFilter>::New();
            triangle_filter->SetInputData(input_polydata);

            auto face_sampler = vtkSmartPointer<vtkPolyDataPointSampler>::New();
            face_sampler->SetInputConnection(triangle_filter->GetOutputPort());
            face_sampler->SetDistance(distance);
            face_sampler->SetGenerateVertexPoints(false);
            face_sampler->SetGenerateEdgePoints(false);
            face_sampler->SetGenerateInteriorPoints(true);
            face_sampler->SetInterpolatePointData(interpolate_point_data);

            face_sampler->Update();
            append_poly_data->AddInputData(face_sampler->GetOutput());
        }

        append_poly_data->Update();

        auto filter_output = append_poly_data->GetOutput();
        output_polydata->ShallowCopy(filter_output);
        return kOfxStatOK;
    }
};

// ----------------------------------------------------------------------------

class VtkVolumePointSamplerEffect : public VtkEffect {
private:
    const char *PARAM_NUMBER_OF_POINTS = "NumberOfPoints";
    const char *PARAM_DISTRIBUTE_UNIFORMLY = "DistributeUniformly";
    const char *PARAM_AUTO_SIMPLIFY = "AutoSimplify";

public:
    const char* GetName() override {
        return "Volume sampling";
    }

    OfxStatus vtkDescribe(OfxParamSetHandle parameters) override {
        AddParam(PARAM_NUMBER_OF_POINTS, 200).Range(1, 1e6).Label("Number of points");
        AddParam(PARAM_DISTRIBUTE_UNIFORMLY, true).Label("Distribute points uniformly");
        AddParam(PARAM_AUTO_SIMPLIFY, true).Label("Auto simplify input mesh");
        // TODO more controls
        return kOfxStatOK;
    }

    OfxStatus vtkCook(vtkPolyData *input_polydata, vtkPolyData *output_polydata) override {
        auto number_of_points = GetParam<int>(PARAM_NUMBER_OF_POINTS).GetValue();
        auto distribute_uniformly = GetParam<bool>(PARAM_DISTRIBUTE_UNIFORMLY).GetValue();
        auto auto_simplify = GetParam<bool>(PARAM_AUTO_SIMPLIFY).GetValue();

        return vtkCook_inner(input_polydata, output_polydata, number_of_points, distribute_uniformly, auto_simplify);
    }

    static OfxStatus vtkCook_inner(vtkPolyData *input_polydata, vtkPolyData *output_polydata,
                                   int number_of_points, bool distribute_uniformly, bool auto_simplify,
                                   bool _assume_input_polydata_triangles=false) {
        double bounds[6];
        input_polydata->GetBounds(bounds);

        auto poly_data_distance = vtkSmartPointer<vtkImplicitPolyDataDistance>::New();

        if (auto_simplify && input_polydata->GetNumberOfPolys() > 100) {
            vtkSmartPointer<vtkPolyData> input_triangle_mesh = input_polydata;
            if (!_assume_input_polydata_triangles) {
                auto triangle_filter = vtkSmartPointer<vtkTriangleFilter>::New();
                triangle_filter->SetInputData(input_polydata);
                triangle_filter->Update();
                input_triangle_mesh = triangle_filter->GetOutput();
            }

            auto input_polycount = input_triangle_mesh->GetNumberOfPolys();
            int target_polycount = 1000 + static_cast<int>(std::sqrt(input_polycount));
            double target_reduction = static_cast<double>(target_polycount) / input_polycount;

            if (target_reduction < 1) {
                auto quadratic_decimation_filter = vtkSmartPointer<vtkQuadricDecimation>::New();
                quadratic_decimation_filter->SetInputData(input_triangle_mesh);
                quadratic_decimation_filter->SetTargetReduction(target_reduction);
                quadratic_decimation_filter->Update();
                poly_data_distance->SetInput(quadratic_decimation_filter->GetOutput());
            } else {
                poly_data_distance->SetInput(input_polydata);
            }
        } else {
            poly_data_distance->SetInput(input_polydata);
        }

        auto points = vtkSmartPointer<vtkPoints>::New();
        points->SetNumberOfPoints(number_of_points);

        output_polydata->SetPoints(points);

        auto distance_arr = vtkSmartPointer<vtkFloatArray>::New();
        distance_arr->SetName("distance");
        distance_arr->SetNumberOfComponents(1);
        distance_arr->SetNumberOfTuples(number_of_points);

        output_polydata->GetPointData()->AddArray(distance_arr);

        auto random_generator_vtk = vtkSmartPointer<vtkMinimalStandardRandomSequence>::New();
        auto random_generator_custom = AdditiveRecurrence<3>();
        auto get_random_uniform = [distribute_uniformly, &random_generator_vtk, &random_generator_custom](int i, double low, double high) -> double {
            if (distribute_uniformly) {
                random_generator_custom.Next();
                return random_generator_custom.GetRangeValue(i, low, high);
            } else {
                random_generator_vtk->Next();
                return random_generator_vtk->GetRangeValue(low, high);
            };
        };

        int i, iteration_count;
        for (i = 0, iteration_count = 0;
             i < number_of_points && iteration_count < 10*number_of_points;
             iteration_count++)
        {
            double x = get_random_uniform(0, bounds[0], bounds[1]),
                   y = get_random_uniform(1, bounds[2], bounds[3]),
                   z = get_random_uniform(2, bounds[4], bounds[5]);

            double distance = poly_data_distance->EvaluateFunction(x, y, z);

            // XXX what side?
            if (distance < 0) {
                points->SetPoint(i, x, y, z);
                distance_arr->SetValue(i, distance);
                i++;
            }
        }

        if (i < number_of_points) {
            printf("WARNING - gave up after %d iterations, but I only have %d points\n", iteration_count, i);
            points->SetNumberOfPoints(i);
            distance_arr->SetNumberOfTuples(i);
        }

        // TODO write error if input has no faces
        // TODO use output distance array

        return kOfxStatOK;
    }
};

// ----------------------------------------------------------------------------

class VtkMaskPointsEffect : public VtkEffect {
private:
    const char *PARAM_RANDOM_MODE = "RandomMode";
    const char *PARAM_RANDOM_MODE_TYPE = "RandomModeType";
    const char *PARAM_ON_RATIO = "OnRatio";
    const char *PARAM_MAXIMUM_NUMBER_OF_POINTS = "MaximumNumberOfPoints";
    // const char *PARAM_RANDOM_SEED = "RandomSeed";

public:
    const char* GetName() override {
        return "Mask points";
    }

    OfxStatus vtkDescribe(OfxParamSetHandle parameters) override {
        AddParam(PARAM_RANDOM_MODE, true).Label("Use point selection");
        AddParam(PARAM_RANDOM_MODE_TYPE, 0).Range(0, 3).Label("Random distribution type"); // TODO replace this with enum
        AddParam(PARAM_ON_RATIO, 2).Label("Take every n-th point");
        AddParam(PARAM_MAXIMUM_NUMBER_OF_POINTS, 10000).Range(0, 10000000).Label("Maximum number of points");
        // AddParam(PARAM_RANDOM_SEED, 1).Label("Random seed");
        return kOfxStatOK;
    }

    OfxStatus vtkCook(vtkPolyData *input_polydata, vtkPolyData *output_polydata) override {
        auto use_random_mode = GetParam<bool>(PARAM_RANDOM_MODE).GetValue();
        auto random_mode_type = GetParam<int>(PARAM_RANDOM_MODE_TYPE).GetValue();
        auto on_ratio = GetParam<int>(PARAM_ON_RATIO).GetValue();
        auto maximum_number_of_points = GetParam<int>(PARAM_MAXIMUM_NUMBER_OF_POINTS).GetValue();
        // auto random_seed = GetParam<int>(PARAM_RANDOM_SEED).GetValue();

        // FIXME until we have eunm, clamp the value manually
        random_mode_type = std::max(0, std::min(3, random_mode_type));

        auto mask_points_filter = vtkSmartPointer<vtkMaskPoints>::New();
        mask_points_filter->SetInputData(input_polydata);

        mask_points_filter->SetRandomMode(use_random_mode);
        mask_points_filter->SetRandomModeType(random_mode_type);
        mask_points_filter->SetOnRatio(on_ratio);
        mask_points_filter->SetMaximumNumberOfPoints(maximum_number_of_points);
        // mask_points_filter->SetRandomSeed(random_seed); // TODO does not exist?

        mask_points_filter->Update();

        auto filter_output = mask_points_filter->GetOutput();
        output_polydata->ShallowCopy(filter_output);
        return kOfxStatOK;
    }
};

// ----------------------------------------------------------------------------

class VtkFeatureEdgesEffect : public VtkEffect {
private:
    const char *PARAM_FEATURE_ANGLE = "FeatureAngle";
    const char *PARAM_FEATURE_EDGES = "FeatureEdges";
    const char *PARAM_BOUNDARY_EDGES = "BoundaryEdges";
    const char *PARAM_NONMANIFOLD_EDGES = "NonManifoldEdges";
    const char *PARAM_MANIFOLD_EDGES = "ManifoldEdges";

public:
    const char* GetName() override {
        return "Feature edges";
    }

    OfxStatus vtkDescribe(OfxParamSetHandle parameters) override {
        AddParam(PARAM_FEATURE_ANGLE, 30).Range(1e-6, 180.0).Label("Feature angle");
        AddParam(PARAM_FEATURE_EDGES, true).Label("Extract feature edges");
        AddParam(PARAM_BOUNDARY_EDGES, false).Label("Extract boundary edges");
        AddParam(PARAM_NONMANIFOLD_EDGES, false).Label("Extract non-manifold edges");
        AddParam(PARAM_MANIFOLD_EDGES, false).Label("Extract manifold edges");
        return kOfxStatOK;
    }

    OfxStatus vtkCook(vtkPolyData *input_polydata, vtkPolyData *output_polydata) override {
        double feature_angle = GetParam<double>(PARAM_FEATURE_ANGLE).GetValue();
        bool extract_feature_edges = GetParam<bool>(PARAM_FEATURE_EDGES).GetValue();
        bool extract_boundary_edges = GetParam<bool>(PARAM_BOUNDARY_EDGES).GetValue();
        bool extract_nonmanifold_edges = GetParam<bool>(PARAM_NONMANIFOLD_EDGES).GetValue();
        bool extract_manifold_edges = GetParam<bool>(PARAM_MANIFOLD_EDGES).GetValue();

        auto filter = vtkSmartPointer<vtkFeatureEdges>::New();
        filter->SetInputData(input_polydata);

        filter->SetFeatureAngle(feature_angle);
        filter->SetFeatureEdges(extract_feature_edges);
        filter->SetBoundaryEdges(extract_boundary_edges);
        filter->SetNonManifoldEdges(extract_nonmanifold_edges);
        filter->SetManifoldEdges(extract_manifold_edges);
        filter->SetColoring(false);

        filter->Update();

        // TODO add cleanpolydata to get rid of unused points

        auto filter_output = filter->GetOutput();
        output_polydata->ShallowCopy(filter_output);
        return kOfxStatOK;
    }
};

// ----------------------------------------------------------------------------

class VtkDelaunay3DEffect : public VtkEffect {
private:
    // const char *PARAM_ALPHA = "Alpha"; // doesn't work very well in my experience
    const char *PARAM_EXTRACT_SURFACE = "ExtractSurface";
    const char *PARAM_EXTRACT_WIREFRAME = "ExtractWireframe";
    // const char *PARAM_ALPHA_TETS = "AlphaTets";
    // const char *PARAM_ALPHA_TRIS = "AlphaTris";
    // const char *PARAM_ALPHA_LINES = "AlphaLines";
    // const char *PARAM_ALPHA_VERTS = "AlphaVerts";

public:
    const char* GetName() override {
        return "Delaunay 3D";
    }

    OfxStatus vtkDescribe(OfxParamSetHandle parameters) override {
        // AddParam(PARAM_ALPHA, 0.0).Range(0, 1e6).Label("Alpha distance");
        AddParam(PARAM_EXTRACT_SURFACE, false).Label("Extract boundary faces (convex hull)");
        AddParam(PARAM_EXTRACT_WIREFRAME, true).Label("Extract tetrahedral edges");
        // AddParam(PARAM_ALPHA_TETS, true).Label("Extract tetrahedra for alpha > 0");
        // AddParam(PARAM_ALPHA_TRIS, true).Label("Extract triangles for alpha > 0");
        // AddParam(PARAM_ALPHA_LINES, true).Label("Extract lines for alpha > 0");
        // AddParam(PARAM_ALPHA_VERTS, false).Label("Extract vertices for alpha > 0");
        return kOfxStatOK;
    }

    OfxStatus vtkCook(vtkPolyData *input_polydata, vtkPolyData *output_polydata) override {
        // auto alpha = GetParam<double>(PARAM_ALPHA).GetValue();
        auto extract_surface = GetParam<bool>(PARAM_EXTRACT_SURFACE).GetValue();
        auto extract_wireframe = GetParam<bool>(PARAM_EXTRACT_WIREFRAME).GetValue();
        // auto alpha_tets = GetParam<bool>(PARAM_ALPHA_TETS).GetValue();
        // auto alpha_tris = GetParam<bool>(PARAM_ALPHA_TRIS).GetValue();
        // auto alpha_lines = GetParam<bool>(PARAM_ALPHA_LINES).GetValue();
        // auto alpha_verts = GetParam<bool>(PARAM_ALPHA_VERTS).GetValue();

        auto delaunay3d_filter  = vtkSmartPointer<vtkDelaunay3D>::New();
        delaunay3d_filter->SetInputData(input_polydata);
        delaunay3d_filter->SetAlpha(0);
        // delaunay3d_filter->SetAlpha(alpha);
        // delaunay3d_filter->SetAlphaTets(alpha_tets);
        // delaunay3d_filter->SetAlphaTris(alpha_tris);
        // delaunay3d_filter->SetAlphaLines(alpha_lines);
        // delaunay3d_filter->SetAlphaVerts(alpha_verts);

        auto append_poly_data = vtkSmartPointer<vtkAppendPolyData>::New();

        if (extract_surface) {
            auto geometry_filter = vtkSmartPointer<vtkGeometryFilter>::New();
            geometry_filter->SetInputConnection(delaunay3d_filter->GetOutputPort());
            // geometry_filter->SetFastMode(true); // try this when it lands in VTK
            geometry_filter->Update();
            append_poly_data->AddInputData(geometry_filter->GetOutput());
        }

        if (extract_wireframe) {
            auto extract_edges = vtkSmartPointer<vtkExtractEdges>::New();
            extract_edges->SetInputConnection(delaunay3d_filter->GetOutputPort());
            extract_edges->Update();
            append_poly_data->AddInputData(extract_edges->GetOutput());
        }

        append_poly_data->Update();
        auto filter_output = append_poly_data->GetOutput();
        output_polydata->ShallowCopy(filter_output);
        return kOfxStatOK;
    }
};

// ----------------------------------------------------------------------------

class VtkTubeFilterEffect : public VtkEffect {
private:
    const char *PARAM_RADIUS = "Radius";
    const char *PARAM_NUMBER_OF_SIDES = "NumberOfSides";
    const char *PARAM_CAPPING = "Capping";
    // const char *PARAM_TUBE_BENDER = "TubeBender";

public:
    const char* GetName() override {
        return "Tube filter";
    }

    OfxStatus vtkDescribe(OfxParamSetHandle parameters) override {
        AddParam(PARAM_RADIUS, 0.05).Range(1e-6, 1e6).Label("Radius");
        AddParam(PARAM_NUMBER_OF_SIDES, 6).Range(3, 1000).Label("Number of sides");
        AddParam(PARAM_CAPPING, true).Label("Cap ends");
        // AddParam(PARAM_TUBE_BENDER, true).Label("Use tube bender");
        // TODO texture coordinates
        return kOfxStatOK;
    }

    OfxStatus vtkCook(vtkPolyData *input_polydata, vtkPolyData *output_polydata) override {
        double radius = GetParam<double>(PARAM_RADIUS).GetValue();
        int number_of_sides = GetParam<int>(PARAM_NUMBER_OF_SIDES).GetValue();
        bool capping = GetParam<bool>(PARAM_CAPPING).GetValue();
        // bool use_tube_bender = GetParam<bool>(PARAM_TUBE_BENDER).GetValue();

        auto append_poly_data = vtkSmartPointer<vtkAppendPolyData>::New();

        if (input_polydata->GetNumberOfPolys() > 0) {
            // vtkExtractEdges to create lines even from polygonal mesh
            auto extract_edges_filter = vtkSmartPointer<vtkExtractEdges>::New();
            extract_edges_filter->SetInputData(input_polydata);
            extract_edges_filter->Update();
            append_poly_data->AddInputData(extract_edges_filter->GetOutput());
        } else {
            append_poly_data->AddInputData(input_polydata);
        }

        // TODO incorporate optional vtkTubeBender - when it lands post VTK 9.0
        // if (use_tube_bender) {
        //    auto tube_bender_filter = vtkSmartPointer<vtkTubeBender>::New();
        //    tube_bender_filter->SetInputConnection(extract_edges_filter->GetOutputPort());
        //    tube_filter_input = tube_bender_filter->GetOutputPort();
        // }

        // vtkTubeFilter to turn lines into polygonal tubes
        auto tube_filter = vtkSmartPointer<vtkTubeFilter>::New();
        tube_filter->SetInputConnection(append_poly_data->GetOutputPort());
        tube_filter->SetRadius(radius);
        tube_filter->SetNumberOfSides(number_of_sides);
        tube_filter->SetCapping(capping);
        tube_filter->SetSidesShareVertices(true);

        // vtkTriangleFilter to convert triangle strips to polygons
        auto triangle_filter = vtkSmartPointer<vtkTriangleFilter>::New();
        triangle_filter->SetInputConnection(tube_filter->GetOutputPort());

        triangle_filter->Update();

        auto filter_output = triangle_filter->GetOutput();
        output_polydata->ShallowCopy(filter_output);
        return kOfxStatOK;
    }
};

// ----------------------------------------------------------------------------

class VtkDecimateProEffect : public VtkEffect {
private:
    const char *PARAM_TARGET_REDUCTION = "TargetReduction";
    const char *PARAM_PRESERVE_TOPOLOGY = "PreserveTopology";
    const char *PARAM_FEATURE_ANGLE = "FeatureAngle";
    const char *PARAM_SPLITTING = "Splitting";
    const char *PARAM_SPLIT_ANGLE = "SplitAngle";
    const char *PARAM_MAXIMUM_ERROR = "MaximumError";
    const char *PARAM_ABSOLUTE_ERROR = "AbsoluteError";
    const char *PARAM_BOUNDARY_VERTEX_DELETION = "BoundaryVertexDeletion";
    const char *PARAM_INFLECTION_POINT_RATIO = "InflectionPointRatio";
    const char *PARAM_DEGREE = "Degree";

public:
    const char* GetName() override {
        return "Decimate (pro)";
    }

    OfxStatus vtkDescribe(OfxParamSetHandle parameters) override {
        AddParam(PARAM_TARGET_REDUCTION, 0.8).Range(0, 1 - 1e-6).Label("Target reduction");
        AddParam(PARAM_PRESERVE_TOPOLOGY, false).Label("Preserve topology");
        AddParam(PARAM_FEATURE_ANGLE, 15.0).Range(0.001, 180.0).Label("Feature angle");
        AddParam(PARAM_SPLITTING, true).Label("Allow splitting");
        AddParam(PARAM_SPLIT_ANGLE, 45.0).Range(0.001, 180.0).Label("Split angle");
        AddParam(PARAM_MAXIMUM_ERROR, 0.01).Range(0, 1e6).Label("Maximum error");
        AddParam(PARAM_ABSOLUTE_ERROR, false).Label("Use absolute error");
        AddParam(PARAM_BOUNDARY_VERTEX_DELETION, true).Label("Allow boundary vertex deletion");
        AddParam(PARAM_INFLECTION_POINT_RATIO, 10.0).Range(1.001, 1e6).Label("Inflection point ratio");
        AddParam(PARAM_DEGREE, 25).Range(3, 1000).Label("Maximum degree of vertex");
        return kOfxStatOK;
    }

    bool vtkIsIdentity(OfxParamSetHandle parameters) override {
        double target_reduction = GetParam<double>(PARAM_TARGET_REDUCTION).GetValue();
        return not is_positive_double(target_reduction);
    }

    OfxStatus vtkCook(vtkPolyData *input_polydata, vtkPolyData *output_polydata) override {
        double target_reduction = GetParam<double>(PARAM_TARGET_REDUCTION).GetValue();
        bool preserve_topology = GetParam<bool>(PARAM_PRESERVE_TOPOLOGY).GetValue();
        double feature_angle = GetParam<double>(PARAM_FEATURE_ANGLE).GetValue();
        bool splitting = GetParam<bool>(PARAM_SPLITTING).GetValue();
        double split_angle = GetParam<double>(PARAM_SPLIT_ANGLE).GetValue();
        double maximum_error = GetParam<double>(PARAM_MAXIMUM_ERROR).GetValue();
        bool absolute_error = GetParam<bool>(PARAM_ABSOLUTE_ERROR).GetValue();
        bool boundary_vertex_deletion = GetParam<bool>(PARAM_BOUNDARY_VERTEX_DELETION).GetValue();
        double inflection_point_ratio = GetParam<double>(PARAM_INFLECTION_POINT_RATIO).GetValue();
        int degree = GetParam<int>(PARAM_DEGREE).GetValue();

        // vtkTriangleFilter to ensure triangle mesh on input
        auto triangle_filter = vtkSmartPointer<vtkTriangleFilter>::New();
        triangle_filter->SetInputData(input_polydata);

        // vtkDecimatePro for main processing
        auto decimate_filter = vtkSmartPointer<vtkDecimatePro>::New();
        decimate_filter->SetInputConnection(triangle_filter->GetOutputPort());
        decimate_filter->SetTargetReduction(target_reduction);
        decimate_filter->SetPreserveTopology(preserve_topology);
        decimate_filter->SetFeatureAngle(feature_angle);
        decimate_filter->SetSplitting(splitting);
        decimate_filter->SetSplitAngle(split_angle);
        decimate_filter->SetMaximumError(maximum_error);
        decimate_filter->SetAbsoluteError(absolute_error);
        decimate_filter->SetBoundaryVertexDeletion(boundary_vertex_deletion);
        decimate_filter->SetInflectionPointRatio(inflection_point_ratio);
        decimate_filter->SetDegree(degree);

        decimate_filter->Update();

        auto filter_output = decimate_filter->GetOutput();
        output_polydata->ShallowCopy(filter_output);
        return kOfxStatOK;
    }
};

// ----------------------------------------------------------------------------

class VtkQuadricDecimationEffect : public VtkEffect {
private:
    const char *PARAM_TARGET_REDUCTION = "TargetReduction";
    const char *PARAM_VOLUME_PRESERVATION = "PreserveTopology";

public:
    const char* GetName() override {
        return "Decimate (quadric)";
    }

    OfxStatus vtkDescribe(OfxParamSetHandle parameters) override {
        AddParam(PARAM_TARGET_REDUCTION, 0.8).Range(0, 1 - 1e-6).Label("Target reduction");
        AddParam(PARAM_VOLUME_PRESERVATION, false).Label("Preserve volume");
        return kOfxStatOK;
    }

    bool vtkIsIdentity(OfxParamSetHandle parameters) override {
        double target_reduction = GetParam<double>(PARAM_TARGET_REDUCTION).GetValue();
        return not is_positive_double(target_reduction);
    }

    OfxStatus vtkCook(vtkPolyData *input_polydata, vtkPolyData *output_polydata) override {
        double target_reduction = GetParam<double>(PARAM_TARGET_REDUCTION).GetValue();
        bool volume_preservation = GetParam<bool>(PARAM_VOLUME_PRESERVATION).GetValue();

        // vtkTriangleFilter to ensure triangle mesh on input
        auto triangle_filter = vtkSmartPointer<vtkTriangleFilter>::New();
        triangle_filter->SetInputData(input_polydata);

        // vtkQuadricDecimation for main processing
        auto decimate_filter = vtkSmartPointer<vtkQuadricDecimation>::New();
        decimate_filter->SetInputConnection(triangle_filter->GetOutputPort());
        decimate_filter->SetTargetReduction(target_reduction);
        decimate_filter->SetVolumePreservation(volume_preservation);
        // TODO the filter supports optimizing for attribute error, too, we could expose this

        decimate_filter->Update();

        auto filter_output = decimate_filter->GetOutput();
        output_polydata->ShallowCopy(filter_output);
        return kOfxStatOK;
    }
};

// ----------------------------------------------------------------------------

// TODO replace vtkQuadricClustering with this once it lands post VTK 9.0
//class VtkBinnedDecimationEffect : public VtkEffect {
//private:
//    const char *PARAM_NUMBER_OF_DIVISIONS = "NumberOfDivisions";
//    const char *PARAM_AUTO_ADJUST_NUMBER_OF_DIVISIONS = "AutoAdjustNumberOfDivisions";
//    const char *PARAM_POINT_GENERATION_MODE = "PointGenerationMode";
//
//public:
//    const char* GetName() override {
//        return "Decimate (binned)";
//    }
//
//    OfxStatus vtkDescribe(OfxParamSetHandle parameters) override {
//        AddParam(PARAM_NUMBER_OF_DIVISIONS, std::array<int,3>{256, 256, 256})
//            .Range({2, 2, 2}, {0xffff, 0xffff, 0xffff})
//            .Label("Number of divisions");
//        AddParam(PARAM_AUTO_ADJUST_NUMBER_OF_DIVISIONS, true).Label("Auto adjust number of divisions");
//        AddParam(PARAM_POINT_GENERATION_MODE, 4).Label("Point generation mode (1-4)"); // TODO make this an enum
//        return kOfxStatOK;
//    }
//
//    OfxStatus vtkCook(vtkPolyData *input_polydata, vtkPolyData *output_polydata) override {
//        auto number_of_divisions = GetParam<std::array<int,3>>(PARAM_NUMBER_OF_DIVISIONS).GetValue();
//        bool auto_adjust_number_of_divisions = GetParam<bool>(PARAM_AUTO_ADJUST_NUMBER_OF_DIVISIONS).GetValue();
//        bool point_generation_mode = GetParam<int>(PARAM_POINT_GENERATION_MODE).GetValue();
//
//        // TODO do we need this?
//        // vtkTriangleFilter to ensure triangle mesh on input
//        //auto triangle_filter = vtkSmartPointer<vtkTriangleFilter>::New();
//        //triangle_filter->SetInputData(input_polydata);
//
//        // vtkBinnedDecimation for main processing
//        auto decimate_filter = vtkSmartPointer<vtkBinnedDecimation>::New();
//        decimate_filter->SetInputData(input_polydata);
//        decimate_filter->SetNumberOfDivisions(number_of_divisions[0], number_of_divisions[1], number_of_divisions[2]);
//        decimate_filter->SetAutoAdjustNumberOfDivisions(auto_adjust_number_of_divisions);
//        decimate_filter->SetPointGenerationMode(point_generation_mode);
//
//        decimate_filter->Update();
//
//        auto filter_output = decimate_filter->GetOutput();
//        output_polydata->ShallowCopy(filter_output);
//        return kOfxStatOK;
//    }
//};

// ----------------------------------------------------------------------------

class VtkQuadricClusteringEffect : public VtkEffect {
private:
    const char *PARAM_NUMBER_OF_DIVISIONS = "NumberOfDivisions";
    const char *PARAM_AUTO_ADJUST_NUMBER_OF_DIVISIONS = "AutoAdjustNumberOfDivisions";

public:
    const char* GetName() override {
        return "Decimate (quadratic clustering)";
    }

    OfxStatus vtkDescribe(OfxParamSetHandle parameters) override {
        AddParam(PARAM_NUMBER_OF_DIVISIONS, std::array<int,3>{256, 256, 256})
            .Range({2, 2, 2}, {0xffff, 0xffff, 0xffff})
            .Label("Number of divisions");
        AddParam(PARAM_AUTO_ADJUST_NUMBER_OF_DIVISIONS, true).Label("Auto adjust number of divisions");
        return kOfxStatOK;
    }

    OfxStatus vtkCook(vtkPolyData *input_polydata, vtkPolyData *output_polydata) override {
        auto number_of_divisions = GetParam<std::array<int,3>>(PARAM_NUMBER_OF_DIVISIONS).GetValue();
        bool auto_adjust_number_of_divisions = GetParam<bool>(PARAM_AUTO_ADJUST_NUMBER_OF_DIVISIONS).GetValue();

        auto decimate_filter = vtkSmartPointer<vtkQuadricClustering>::New();
        decimate_filter->SetInputData(input_polydata);
        decimate_filter->SetNumberOfDivisions(number_of_divisions[0], number_of_divisions[1], number_of_divisions[2]);
        decimate_filter->SetAutoAdjustNumberOfDivisions(auto_adjust_number_of_divisions);

        decimate_filter->Update();

        auto filter_output = decimate_filter->GetOutput();
        output_polydata->ShallowCopy(filter_output);
        return kOfxStatOK;
    }
};

// ----------------------------------------------------------------------------

class VtkFillHolesEffect : public VtkEffect {
private:
    const char *PARAM_HOLE_SIZE = "HoleSize";

public:
    const char* GetName() override {
        return "Fill holes";
    }

    OfxStatus vtkDescribe(OfxParamSetHandle parameters) override {
        AddParam(PARAM_HOLE_SIZE, 1.0).Range(0, 1e6).Label("Maximum hole size");
        return kOfxStatOK;
    }

    bool vtkIsIdentity(OfxParamSetHandle parameters) override {
        double hole_size = GetParam<double>(PARAM_HOLE_SIZE).GetValue();
        printf("VtkFillHolesEffect::vtkIsIdentity() - hole_size=%g, is it less than %g?", hole_size, DBL_EPSILON);
        return not is_positive_double(hole_size);
    }

    OfxStatus vtkCook(vtkPolyData *input_polydata, vtkPolyData *output_polydata) override {
        double hole_size = GetParam<double>(PARAM_HOLE_SIZE).GetValue();

        auto filter = vtkSmartPointer<vtkFillHolesFilter>::New();
        filter->SetInputData(input_polydata);

        filter->SetHoleSize(hole_size);

        filter->Update();

        auto filter_output = filter->GetOutput();
        output_polydata->ShallowCopy(filter_output);
        return kOfxStatOK;
    }
};

// ----------------------------------------------------------------------------

class VtkIdentityEffect : public VtkEffect {
public:
    const char *PARAM_ACTION_IS_IDENTITY = "ActionIsIdentity";

    const char* GetName() override {
        return "Identity";
    }

    OfxStatus vtkDescribe(OfxParamSetHandle parameters) override {
        AddParam(PARAM_ACTION_IS_IDENTITY, false).Label("kOfxMeshEffectActionIsIdentity");
        return kOfxStatOK;
    }

    bool vtkIsIdentity(OfxParamSetHandle parameters) override {
        bool action_is_identity = GetParam<bool>(PARAM_ACTION_IS_IDENTITY).GetValue();
        return action_is_identity;
    }

    OfxStatus vtkCook(vtkPolyData *input_polydata, vtkPolyData *output_polydata) override {
        output_polydata->ShallowCopy(input_polydata);
        return kOfxStatOK;
    }
};

// ----------------------------------------------------------------------------

MfxRegister(
        VtkSmoothPolyDataFilterEffect,
        VtkWindowedSincPolyDataFilterEffect,
        VtkPolyDataPointSamplerEffect,
        VtkMaskPointsEffect,
        VtkFeatureEdgesEffect,
        VtkVolumePointSamplerEffect,
        VtkDelaunay3DEffect,
        VtkFillHolesEffect,
        VtkTubeFilterEffect,
        VtkQuadricDecimationEffect,
        VtkDecimateProEffect,
        VtkQuadricClusteringEffect,

        // these effects are interesting only for development of Open Mesh Effect
        VtkIdentityEffect
);
