/*
 * State machine, based on "Patterns in C - Part 2: STATE" by Adam Petersen
 * http://www.adampetersen.se/Patterns%20in%20C%202,%20STATE.pdf
 *
 * This application uses the Pololu AVR C/C++ Library.  For help, see:
 * -User's guide: http://www.pololu.com/docs/0J20
 * -Command reference: http://www.pololu.com/docs/0J18
 */
#include <string.h>
#include <rob_system.h>
#include <rob_wrappers.h>

#include <rob_home.h>
#include <rob_home_state_machine.h>
#include <rob_home_state_machine_events.h>

/*
 * MANIFEST CONSTANTS
 */

/*
 * TYPES
 */

/*
 * PUBLIC FUNCTIONS
 */
void eventHomeStartOrangutan (HomeContext *pInstance)
{
    ASSERT_PARAM (pInstance != PNULL, 0);
    pInstance->state.pEventHomeStart (&(pInstance->state));
}

void eventHomeRoughIntegrationDoneOrangutan (HomeContext *pInstance)
{
    ASSERT_PARAM (pInstance != PNULL, 0);
    pInstance->state.pEventHomeRoughIntegrationDone (&(pInstance->state));
}

void eventHomeRoughAlignmentDoneOrangutan (HomeContext *pInstance)
{
    ASSERT_PARAM (pInstance != PNULL, 0);
    pInstance->state.pEventHomeRoughAlignmentDone (&(pInstance->state));
}

void eventHomeRoughAlignmentFailedOrangutan (HomeContext *pInstance)
{
    ASSERT_PARAM (pInstance != PNULL, 0);
    pInstance->state.pEventHomeRoughAlignmentFailed (&(pInstance->state));
}

void eventHomeFineIntegrationDoneOrangutan (HomeContext *pInstance)
{
    ASSERT_PARAM (pInstance != PNULL, 0);
    pInstance->state.pEventHomeFineIntegrationDone (&(pInstance->state));
}

void eventHomeFineAlignmentDoneOrangutan (HomeContext *pInstance)
{
    ASSERT_PARAM (pInstance != PNULL, 0);
    pInstance->state.pEventHomeFineAlignmentDone (&(pInstance->state));
}

void eventHomeFineAlignmentFailedOrangutan (HomeContext *pInstance)
{
    ASSERT_PARAM (pInstance != PNULL, 0);
    pInstance->state.pEventHomeFineAlignmentFailed (&(pInstance->state));
}

void eventHomeTravelIntegrationDoneOrangutan (HomeContext *pInstance)
{
    ASSERT_PARAM (pInstance != PNULL, 0);
    pInstance->state.pEventHomeTravelIntegrationDone (&(pInstance->state));
}

void eventHomeTravelAlignmentFailedOrangutan (HomeContext *pInstance)
{
    ASSERT_PARAM (pInstance != PNULL, 0);
    pInstance->state.pEventHomeTravelAlignmentFailed (&(pInstance->state));
}

void eventHomeStopOrangutan (HomeContext *pInstance)
{
    ASSERT_PARAM (pInstance != PNULL, 0);
    pInstance->state.pEventHomeStop (&(pInstance->state));
}