rock_find_pkgconfig(EIGEN3 eigen3 REQUIRED)
find_package(Boost REQUIRED COMPONENTS filesystem thread system)
find_package(OpenCV REQUIRED core imgproc calib3d highgui)
find_package(Ceres REQUIRED)
find_package(PCL 1.7 REQUIRED COMPONENTS common filters)

rock_library(eds
    SOURCES
        bundles/AccumulatedSCHessian.cpp
        bundles/AccumulatedTopHessian.cpp
        bundles/EnergyFunctional.cpp
        bundles/EnergyFunctionalStructs.cpp
        init/CoarseInitializer.cpp
        io/ImageRW.cpp
        io/ImageConvert.cpp
        io/OutputMaps.cpp
        mapping/PixelSelector.cpp
        mapping/DepthPoints.cpp
        tracking/CoarseTracker.cpp
        tracking/EventFrame.cpp
        tracking/HessianBlocks.cpp
        tracking/ImmaturePoint.cpp
        tracking/KeyFrame.cpp
        tracking/Residuals.cpp
        tracking/Tracker.cpp
        utils/Calib.cpp
        utils/Colormap.cpp
        utils/globalCalib.cpp
        utils/settings.cpp
        utils/Undistort.cpp
        utils/Utils.cpp

    HEADERS
        bundles/Config.hpp
        bundles/AccumulatedSCHessian.h
        bundles/AccumulatedTopHessian.h
        bundles/EnergyFunctional.h
        bundles/EnergyFunctionalStructs.h
        bundles/MatrixAccumulators.h
        bundles/RawResidualJacobian.h
        init/CoarseInitializer.h
        io/ImageRW.h
        io/ImageConvert.h
        io/OutputMaps.h
        mapping/Config.hpp
        mapping/DepthPoints.hpp
        mapping/Types.hpp
        mapping/PixelSelector.h
        sophus/rxso3.hpp
        sophus/se2.hpp
        sophus/se3.hpp
        sophus/sim3.hpp
        sophus/so2.hpp
        sophus/so3.hpp
        sophus/sophus.hpp
        tracking/Config.hpp
        tracking/EventFrame.hpp
        tracking/KeyFrame.hpp
        tracking/PhotometricError.hpp
        tracking/PhotometricErrorNC.hpp
        tracking/Tracker.hpp
        tracking/Types.hpp
        tracking/CoarseTracker.h
        tracking/HessianBlocks.h
        tracking/ImmaturePoint.h
        tracking/ResidualProjections.h
        tracking/Residuals.h
        utils/Calib.hpp
        utils/Colormap.hpp
        utils/Config.hpp
        utils/KDTree.hpp
        utils/Transforms.hpp
        utils/Utils.hpp
        utils/FrameShell.h
        utils/globalCalib.h
        utils/globalFuncs.h
        utils/ImageAndExposure.h
        utils/IndexThreadReduce.h
        utils/MinimalImage.h
        utils/nanoflann.h
        utils/NumType.h
        utils/settings.h
        utils/Undistort.h
        EDS.h
    DEPS_TARGET
        ceres
    DEPS_PKGCONFIG
        base-types
        base-logging
        yaml-cpp
        frame_helper
        pcl_common-${PCL_VERSION_MAJOR}.${PCL_VERSION_MINOR}
        pcl_filters-${PCL_VERSION_MAJOR}.${PCL_VERSION_MINOR}
        ${OPENCV_PACKAGE}
    DEPS_PLAIN Boost_SYSTEM Boost_FILESYSTEM Boost_THREAD
        NONINSTALL
        )
