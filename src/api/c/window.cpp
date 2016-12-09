/*******************************************************
 * Copyright (c) 2014, ArrayFire
 * All rights reserved.
 *
 * This file is distributed under 3-clause BSD license.
 * The complete license agreement can be obtained at:
 * http://arrayfire.com/licenses/BSD-3-Clause
 ********************************************************/


#include <af/graphics.h>
#include <af/algorithm.h>

#include <graphics_common.hpp>
#include <err_common.hpp>
#include <backend.hpp>

using af::dim4;
using namespace detail;

#if defined(WITH_GRAPHICS)
using namespace graphics;
#endif


af_err af_create_window(af_window *out, const int width, const int height, const char* const title)
{
#if defined(WITH_GRAPHICS)
    forge::Window* wnd;
    try {
        graphics::ForgeManager& fgMngr = graphics::ForgeManager::getInstance();
        forge::Window* mainWnd = NULL;

        try {
            mainWnd = fgMngr.getMainWindow();
        } catch(...) {
            std::cerr<<"OpenGL context creation failed"<<std::endl;
        }

        if(mainWnd==0) {
            std::cerr<<"Not a valid window"<<std::endl;
            return AF_SUCCESS;
        }

        wnd = new forge::Window(width, height, title, mainWnd);
        wnd->setFont(fgMngr.getFont());

        // Create a chart map
        fgMngr.setWindowChartGrid(wnd, 1, 1);

        *out = reinterpret_cast<af_window>(wnd);
    }
    CATCHALL;
    return AF_SUCCESS;
#else
    AF_RETURN_ERROR("ArrayFire compiled without graphics support", AF_ERR_NO_GFX);
#endif
}

af_err af_set_position(const af_window wind, const unsigned x, const unsigned y)
{
#if defined(WITH_GRAPHICS)
    if(wind==0) {
        std::cerr<<"Not a valid window"<<std::endl;
        return AF_SUCCESS;
    }

    try {
        forge::Window* wnd = reinterpret_cast<forge::Window*>(wind);
        wnd->setPos(x, y);
    }
    CATCHALL;
    return AF_SUCCESS;
#else
    AF_RETURN_ERROR("ArrayFire compiled without graphics support", AF_ERR_NO_GFX);
#endif
}

af_err af_set_title(const af_window wind, const char* const title)
{
#if defined(WITH_GRAPHICS)
    if(wind==0) {
        std::cerr<<"Not a valid window"<<std::endl;
        return AF_SUCCESS;
    }

    try {
        forge::Window* wnd = reinterpret_cast<forge::Window*>(wind);
        wnd->setTitle(title);
    }
    CATCHALL;
    return AF_SUCCESS;
#else
    AF_RETURN_ERROR("ArrayFire compiled without graphics support", AF_ERR_NO_GFX);
#endif
}

af_err af_set_size(const af_window wind, const unsigned w, const unsigned h)
{
#if defined(WITH_GRAPHICS)
    if(wind==0) {
        std::cerr<<"Not a valid window"<<std::endl;
        return AF_SUCCESS;
    }

    try {
        forge::Window* wnd = reinterpret_cast<forge::Window*>(wind);
        wnd->setSize(w, h);
    }
    CATCHALL;
    return AF_SUCCESS;
#else
    AF_RETURN_ERROR("ArrayFire compiled without graphics support", AF_ERR_NO_GFX);
#endif
}

af_err af_grid(const af_window wind, const int rows, const int cols)
{
#if defined(WITH_GRAPHICS)
    if(wind==0) {
        std::cerr<<"Not a valid window"<<std::endl;
        return AF_SUCCESS;
    }

    try {
        forge::Window* wnd = reinterpret_cast<forge::Window*>(wind);
        wnd->grid(rows, cols);

        // Recreate a chart map
        ForgeManager& fgMngr = ForgeManager::getInstance();
        fgMngr.setWindowChartGrid(wnd, rows, cols);
    }
    CATCHALL;
    return AF_SUCCESS;
#else
    AF_RETURN_ERROR("ArrayFire compiled without graphics support", AF_ERR_NO_GFX);
#endif
}

// dir -> true = round up, false = round down
double step_round(const double in, const bool dir)
{
    if(in == 0) return 0;

    static const double __log2 = log10(2);
    static const double __log4 = log10(4);
    static const double __log6 = log10(6);
    static const double __log8 = log10(8);

    // log_in is of the form "s abc.xyz", where
    // s is either + or -; + indicates abs(in) >= 1 and - indicates 0 < abs(in) < 1 (log10(1) is +0)
    const double sign   = in < 0 ? -1 : 1;
    const double log_in = std::log10(std::fabs(in));
    const double mag    = std::pow(10, std::floor(log_in)) * sign;  // Number of digits either left or right of 0
    const double dec    = std::log10(in / mag); // log of the fraction

    // This means in is of the for 10^n
    if(dec == 0) return in;

    // For negative numbers, -ve round down = +ve round up and vice versa
    bool op_dir = in > 0 ? dir : !dir;

    double mult = 1;

    // Round up
    if(op_dir) {
        if(dec <= __log2) {
            mult = 2;
        } else if(dec <= __log4) {
            mult = 4;
        } else if(dec <= __log6) {
            mult = 6;
        } else if(dec <= __log8) {
            mult = 8;
        } else {
            mult = 10;
        }
    } else {    // Round down
        if(dec < __log2) {
            mult = 1;
        } else if(dec < __log4) {
            mult = 2;
        } else if(dec < __log6) {
            mult = 4;
        } else if(dec < __log8) {
            mult = 6;
        } else {
            mult = 8;
        }
    }

    return mag * mult;
}

af_err af_set_axes_limits_compute(const af_window wind,
                                  const af_array x, const af_array y, const af_array z,
                                  const bool exact, const af_cell* const props)
{
#if defined(WITH_GRAPHICS)
    if(wind==0) {
        std::cerr<<"Not a valid window"<<std::endl;
        return AF_SUCCESS;
    }

    try {
        forge::Window* window = reinterpret_cast<forge::Window*>(wind);

        // Recreate a chart map
        ForgeManager& fgMngr = ForgeManager::getInstance();

        forge::Chart* chart = NULL;
        // The ctype here below doesn't really matter as it is only fetching
        // the chart. It will not set it.
        // If this is actually being done, then it is extremely bad.
        // But lets have a check anyway.
        fg_chart_type ctype = (z == NULL || z == 0) ? FG_CHART_2D : FG_CHART_3D;

        if (props->col > -1 && props->row > -1)
            chart = fgMngr.getChart(window, props->row, props->col, ctype);
        else
            chart = fgMngr.getChart(window, 0, 0, ctype);

        double xmin = -1, xmax = 1;
        double ymin = -1, ymax = 1;
        double zmin = -1, zmax = 1;
        AF_CHECK(af_min_all(&xmin, NULL, x));
        AF_CHECK(af_max_all(&xmax, NULL, x));
        AF_CHECK(af_min_all(&ymin, NULL, y));
        AF_CHECK(af_max_all(&ymax, NULL, y));

        if(ctype == FG_CHART_3D) {
            AF_CHECK(af_min_all(&zmin, NULL, z));
            AF_CHECK(af_max_all(&zmax, NULL, z));
        }

        if(!exact) {
            xmin = step_round(xmin, false);
            xmax = step_round(xmax, true );
            ymin = step_round(ymin, false);
            ymax = step_round(ymax, true );
            zmin = step_round(zmin, false);
            zmax = step_round(zmax, true );
        }

        chart->setAxesLimits(xmin, xmax, ymin, ymax, zmin, zmax);
    }
    CATCHALL;
    return AF_SUCCESS;
#else
    AF_RETURN_ERROR("ArrayFire compiled without graphics support", AF_ERR_NO_GFX);
#endif
}

af_err af_set_axes_limits_2d(const af_window wind,
                             const float xmin, const float xmax,
                             const float ymin, const float ymax,
                             const bool exact, const af_cell* const props)
{
#if defined(WITH_GRAPHICS)
    if(wind==0) {
        std::cerr<<"Not a valid window"<<std::endl;
        return AF_SUCCESS;
    }

    try {
        forge::Window* window = reinterpret_cast<forge::Window*>(wind);

        // Recreate a chart map
        ForgeManager& fgMngr = ForgeManager::getInstance();

        forge::Chart* chart = NULL;
        // The ctype here below doesn't really matter as it is only fetching
        // the chart. It will not set it.
        // If this is actually being done, then it is extremely bad.
        fg_chart_type ctype = FG_CHART_2D;

        if (props->col > -1 && props->row > -1)
            chart = fgMngr.getChart(window, props->row, props->col, ctype);
        else
            chart = fgMngr.getChart(window, 0, 0, ctype);

        float _xmin = xmin;
        float _xmax = xmax;
        float _ymin = ymin;
        float _ymax = ymax;
        if(!exact) {
            _xmin = step_round(_xmin, false);
            _xmax = step_round(_xmax, true );
            _ymin = step_round(_ymin, false);
            _ymax = step_round(_ymax, true );
        }

        chart->setAxesLimits(_xmin, _xmax, _ymin, _ymax);
    }
    CATCHALL;
    return AF_SUCCESS;
#else
    AF_RETURN_ERROR("ArrayFire compiled without graphics support", AF_ERR_NO_GFX);
#endif
}

af_err af_set_axes_limits_3d(const af_window wind,
                             const float xmin, const float xmax,
                             const float ymin, const float ymax,
                             const float zmin, const float zmax,
                             const bool exact, const af_cell* const props)
{
#if defined(WITH_GRAPHICS)
    if(wind==0) {
        std::cerr<<"Not a valid window"<<std::endl;
        return AF_SUCCESS;
    }

    try {
        forge::Window* window = reinterpret_cast<forge::Window*>(wind);

        // Recreate a chart map
        ForgeManager& fgMngr = ForgeManager::getInstance();

        forge::Chart* chart = NULL;
        // The ctype here below doesn't really matter as it is only fetching
        // the chart. It will not set it.
        // If this is actually being done, then it is extremely bad.
        fg_chart_type ctype = FG_CHART_3D;

        if (props->col > -1 && props->row > -1)
            chart = fgMngr.getChart(window, props->row, props->col, ctype);
        else
            chart = fgMngr.getChart(window, 0, 0, ctype);

        float _xmin = xmin;
        float _xmax = xmax;
        float _ymin = ymin;
        float _ymax = ymax;
        float _zmin = zmin;
        float _zmax = zmax;
        if(!exact) {
            _xmin = step_round(_xmin, false);
            _xmax = step_round(_xmax, true );
            _ymin = step_round(_ymin, false);
            _ymax = step_round(_ymax, true );
            _zmin = step_round(_zmin, false);
            _zmax = step_round(_zmax, true );
        }

        chart->setAxesLimits(_xmin, _xmax, _ymin, _ymax, _zmin, _zmax);
    }
    CATCHALL;
    return AF_SUCCESS;
#else
    AF_RETURN_ERROR("ArrayFire compiled without graphics support", AF_ERR_NO_GFX);
#endif
}

af_err af_set_axes_titles(const af_window wind,
                          const char * const xtitle,
                          const char * const ytitle,
                          const char * const ztitle,
                          const af_cell* const props)
{
#if defined(WITH_GRAPHICS)
    if(wind==0) {
        std::cerr<<"Not a valid window"<<std::endl;
        return AF_SUCCESS;
    }

    try {
        forge::Window* window = reinterpret_cast<forge::Window*>(wind);

        // Recreate a chart map
        ForgeManager& fgMngr = ForgeManager::getInstance();

        forge::Chart* chart = NULL;

        // The ctype here below doesn't really matter as it is only fetching
        // the chart. It will not set it.
        // If this is actually being done, then it is extremely bad.
        // But lets have a check anyway.
        fg_chart_type ctype = (ztitle == NULL || ztitle == 0) ? FG_CHART_2D : FG_CHART_3D;

        if (props->col > -1 && props->row > -1)
            chart = fgMngr.getChart(window, props->row, props->col, ctype);
        else
            chart = fgMngr.getChart(window, 0, 0, ctype);

        chart->setAxesTitles(xtitle, ytitle, ztitle);
    }
    CATCHALL;
    return AF_SUCCESS;
#else
    AF_RETURN_ERROR("ArrayFire compiled without graphics support", AF_ERR_NO_GFX);
#endif
}

af_err af_show(const af_window wind)
{
#if defined(WITH_GRAPHICS)
    if(wind==0) {
        std::cerr<<"Not a valid window"<<std::endl;
        return AF_SUCCESS;
    }

    try {
        forge::Window* wnd = reinterpret_cast<forge::Window*>(wind);
        wnd->swapBuffers();
    }
    CATCHALL;
    return AF_SUCCESS;
#else
    AF_RETURN_ERROR("ArrayFire compiled without graphics support", AF_ERR_NO_GFX);
#endif
}

af_err af_is_window_closed(bool *out, const af_window wind)
{
#if defined(WITH_GRAPHICS)
    if(wind==0) {
        std::cerr<<"Not a valid window"<<std::endl;
        return AF_SUCCESS;
    }

    try {
        forge::Window* wnd = reinterpret_cast<forge::Window*>(wind);
        *out = wnd->close();
    }
    CATCHALL;
    return AF_SUCCESS;
#else
    AF_RETURN_ERROR("ArrayFire compiled without graphics support", AF_ERR_NO_GFX);
#endif
}

af_err af_set_visibility(const af_window wind, const bool is_visible)
{
#if defined(WITH_GRAPHICS)
    if(wind==0) {
        std::cerr<<"Not a valid window"<<std::endl;
        return AF_SUCCESS;
    }

    try {
        forge::Window* wnd = reinterpret_cast<forge::Window*>(wind);
        if (is_visible)
            wnd->show();
        else
            wnd->hide();
    }
    CATCHALL;
    return AF_SUCCESS;
#else
    AF_RETURN_ERROR("ArrayFire compiled without graphics support", AF_ERR_NO_GFX);
#endif
}

af_err af_destroy_window(const af_window wind)
{
#if defined(WITH_GRAPHICS)
    if(wind==0) {
        std::cerr<<"Not a valid window"<<std::endl;
        return AF_SUCCESS;
    }

    try {
        forge::Window* wnd = reinterpret_cast<forge::Window*>(wind);

        // Delete chart map
        ForgeManager& fgMngr = ForgeManager::getInstance();
        fgMngr.setWindowChartGrid(wnd, 0, 0);

        delete wnd;
    }
    CATCHALL;
    return AF_SUCCESS;
#else
    AF_RETURN_ERROR("ArrayFire compiled without graphics support", AF_ERR_NO_GFX);
#endif
}

