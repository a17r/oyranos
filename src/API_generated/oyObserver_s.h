
/** @file oyObserver_s.h

   [Template file inheritance graph]
   +-- oyObserver_s.template.h

 *  Oyranos is an open source Color Management System
 *
 *  @par Copyright:
 *            2004-2016 (C) Kai-Uwe Behrmann
 *
 *  @author   Kai-Uwe Behrmann <ku.b@gmx.de>
 *  @par License:
 *            new BSD - see: http://www.opensource.org/licenses/BSD-3-Clause
 */


#ifndef OY_OBSERVER_S_H
#define OY_OBSERVER_S_H

#ifdef __cplusplus
/*namespace oyranos {*/
  extern "C" {
#endif /* __cplusplus */


#include <oyranos_object.h>
#include <oyranos_definitions.h>

#include "oyStruct_s.h"

#define OY_SIGNAL_BLOCK                0x01 /**< do not send new signals */
#define oyToSignalBlock_m(r)           ((r)&1)
#define OY_SIGNAL_OBSERVERS OY_TOP_SHARED OY_SLASH OY_DOMAIN_INTERNAL OY_SLASH OY_TYPE_STD OY_SLASH "oyStructList_s/observers"
/*  The models list of a observing object is just a clone of the oyObserver_s
 *  object added to the model. */
#define OY_SIGNAL_MODELS OY_TOP_SHARED OY_SLASH OY_DOMAIN_INTERNAL OY_SLASH OY_TYPE_STD OY_SLASH "oyStructList_s/models"

/** @enum    oySIGNAL_e
 *  @brief   observer signals
 *  @ingroup objects_generic
 *
 *  The signal types are similiar to the graph event enum oyCONNECTOR_EVENT_e.
 *
 *  @version Oyranos: 0.1.10
 *  @since   2009/10/26 (Oyranos: 0.1.10)
 *  @date    2009/10/26
 */
typedef enum {
  oySIGNAL_OK,
  oySIGNAL_CONNECTED,                  /**< connection established */
  oySIGNAL_RELEASED,                   /**< released the connection */
  oySIGNAL_DATA_CHANGED,               /**< call to update image views */
  oySIGNAL_STORAGE_CHANGED,            /**< new data accessors */
  oySIGNAL_INCOMPATIBLE_DATA,          /**< can not process image */
  oySIGNAL_INCOMPATIBLE_OPTION,        /**< can not handle option */
  oySIGNAL_INCOMPATIBLE_CONTEXT,       /**< can not handle profile */
  oySIGNAL_USER1,                      /**< user signal type */
  oySIGNAL_USER2,                      /**< user signal type */
  oySIGNAL_USER3                       /**< more signal types are possible */
} oySIGNAL_e;

const char *       oySignalToString  ( oySIGNAL_e          signal_type );

typedef struct oyObserver_s oyObserver_s;

/** @brief   signal handler type
 *
 *  @param[in]     observer            the actual observer
 *  @param[in]     signal_type         the sent signal type
 *  @param[in,out] signal_data         the sent signal data to analyse or manipulate
 *  @return                            error
 *
 *  @version Oyranos: 0.1.10
 *  @date    2009/10/26
 *  @since   2009/10/26 (Oyranos: 0.1.10)
 */
typedef  int      (*oyObserver_Signal_f) (
                                       oyObserver_s      * observer,
                                       oySIGNAL_e          signal_type,
                                       oyStruct_s        * signal_data );


/* Include "Observer.dox" { */
/** @struct   oyObserver_s
 *  @ingroup  objects_generic
 *  @brief    Oyranos object observers
 *
 *  oyObserver_s is following the viewer/model design pattern. The relations of
 *  oyObserver_s' can be anything up to complicated cyclic, directed graphs.
 *  The oyObserver_s type is intented for communication to non graph objects.
 *  Oyranos graphs have several communication paths available, which should
 *  be prefered over oyObserver_s when possible.
 *
 *  The struct contains properties to signal changes to a observer.
 *  The signaling provides a small set of very generic signals types as
 *  enumeration.
 *  It is possible for models to add additional data to the signal. These
 *  additional data is only blindly transported. A agreement is not subject of
 *  the oyObserver_s structure. For completeness the observed object shall
 *  always be included in the signal.
 *
 *  This class is a public members only class and does not
 *  belong to the oyranos object model, like all "Generic Objects".
 *
 *  @note New templates will not be created automaticly [notemplates]
 *
 *  @version Oyranos: 0.1.10
 *  @since   2009/10/26 (Oyranos: 0.1.10)
 *  @date    2009/10/26
 */

/* } Include "Observer.dox" */

struct oyObserver_s {
/* Include "Observer.members.h" { */
  oyOBJECT_e           type_;          /**< @private internal struct type oyOBJECT_OBSERVER_S */
  oyStruct_Copy_f      copy;           /**< copy function */
  oyStruct_Release_f   release;        /**< release function */
  oyPointer            dummy;          /**< keep to zero */

  /** a reference to the observing object */
  oyStruct_s         * observer;
  /** a reference to the to be observed model */
  oyStruct_s         * model;
  /** optional data; If no other user data is available this data will be
   *  passed with the signal. */
  oyStruct_s         * user_data;
  oyObserver_Signal_f  signal;         /**< observers signaling function */
  int                  disable_ref;    /**< disable signals reference counter
                                            == 0 -> enabled; otherwise not */

/* } Include "Observer.members.h" */
};

/* oyObserver_s common object functions { */
OYAPI oyObserver_s * OYEXPORT
           oyObserver_New            ( oyObject_s          object );
OYAPI oyObserver_s * OYEXPORT
           oyObserver_Copy           ( oyObserver_s      * obj,
                                       oyObject_s          object);
OYAPI int  OYEXPORT
           oyObserver_Release        ( oyObserver_s     ** obj );
/* } oyObserver_s common object functions */

/* Include "Observer.public_methods_declarations.h" { */
OYAPI int  OYEXPORT
           oyObserver_SignalSend     ( oyObserver_s      * observer,
                                       oySIGNAL_e          signal_type,
                                       oyStruct_s        * user_data );
OYAPI int  OYEXPORT
           oyStruct_ObserverAdd      ( oyStruct_s        * model,
                                       oyStruct_s        * observer,
                                       oyStruct_s        * user_data,
                                       oyObserver_Signal_f signalFunc );
OYAPI int  OYEXPORT
           oyStruct_ObserverRemove   ( oyStruct_s        * model,
                                       oyStruct_s        * observer,
                                       oyObserver_Signal_f signalFunc );
OYAPI int  OYEXPORT
           oyStruct_ObserverSignal   ( oyStruct_s        * model,
                                       oySIGNAL_e          signal_type,
                                       oyStruct_s        * signal_data );
OYAPI int  OYEXPORT
           oyStruct_DisableSignalSend( oyStruct_s        * model );
OYAPI int  OYEXPORT
           oyStruct_EnableSignalSend ( oyStruct_s        * model );
OYAPI int  OYEXPORT
           oyStruct_ObserversCopy    ( oyStruct_s        * object,
                                       oyStruct_s        * pattern,
                                       uint32_t            flags );
OYAPI int  OYEXPORT
           oyStruct_ObserverCopyModel( oyStruct_s        * model,
                                       oyStruct_s        * pattern,
                                       uint32_t            flags );
OYAPI int  OYEXPORT
           oyStruct_ObserverCopyObserver (
                                       oyStruct_s        * observer,
                                       oyStruct_s        * pattern,
                                       uint32_t            flags );
OYAPI int  OYEXPORT
           oyStruct_IsObserved       ( oyStruct_s        * model,
                                       oyStruct_s        * observer );
OYAPI uint32_t OYEXPORT
           oyObserverGetFlags        ( void );
OYAPI int  OYEXPORT
           oyObserverSetFlags        ( uint32_t            flags );

/* } Include "Observer.public_methods_declarations.h" */

#ifdef __cplusplus
} /* extern "C" */
/*}*/ /* namespace oyranos */
#endif /* __cplusplus */

#endif /* OY_OBSERVER_S_H */
