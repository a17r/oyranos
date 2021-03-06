#include "oyOptions_s.h"
#include "oyCMMapiFilter_s.h"
/** typedef  oyCMMuiGet_f
 *  @brief   obtain a XFORMS ui description
 *  @ingroup module_api
 *  @memberof oyCMMui_s
 *
 *  The structures can provide a XFORMS ui based on the modules own
 *  set of options. The options are in the property of the caller.
 *
 *  @param[in]     module              the owner
 *  @param[in]     options             the options to display
 *  @param[out]    ui_text             the XFORMS string
 *  @param[in]     allocateFunc        user allocator
 *  @return                            0 on success; error >= 1; -1 not understood; unknown < -1
 *
 *  @version Oyranos: 0.9.5
 *  @date    2014/01/08
 *  @since   2009/01/18 (Oyranos: 0.1.10)
 */
typedef int  (*oyCMMuiGet_f)         ( oyCMMapiFilter_s   * module,
                                       oyOptions_s        * options,
                                       char              ** ui_text,
                                       oyAlloc_f            allocateFunc );

