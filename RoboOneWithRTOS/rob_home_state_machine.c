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
 * STATIC FUNCTIONS: DEFAULT EVENT HANDLERS
 */

static void defaultEventHomeStart (HomeState *pState)
{
    rob_print_from_program_space (PSTR ("!eH+ in "));
    rob_print (&(pState->name[0]));
}

static void defaultEventHomeRoughIntegrationDone (HomeState *pState)
{
    rob_print_from_program_space (PSTR ("!eRID in "));
    rob_print (&(pState->name[0]));
}

static void defaultEventHomeRoughAlignmentDone (HomeState *pState)
{
    rob_print_from_program_space (PSTR ("!eRAD in "));
    rob_print (&(pState->name[0]));
}

static void defaultEventHomeRoughAlignmentFailed (HomeState *pState)
{
    rob_print_from_program_space (PSTR ("!eRAF in "));
    rob_print (&(pState->name[0]));
}

static void defaultEventHomeFineIntegrationDone (HomeState *pState)
{
    rob_print_from_program_space (PSTR ("!eFID in "));
    rob_print (&(pState->name[0]));
}

static void defaultEventHomeFineAlignmentDone (HomeState *pState)
{
    rob_print_from_program_space (PSTR ("!eFAD in "));
    rob_print (&(pState->name[0]));
}

static void defaultEventHomeFineAlignmentFailed (HomeState *pState)
{
    rob_print_from_program_space (PSTR ("!eFAF in "));
    rob_print (&(pState->name[0]));
}

static void defaultEventHomeTravelIntegrationDone (HomeState *pState)
{
    rob_print_from_program_space (PSTR ("!eTID in "));
    rob_print (&(pState->name[0]));
}

static void defaultEventHomeTravelAlignmentFailed (HomeState *pState)
{
    rob_print_from_program_space (PSTR ("!eTAF in "));
    rob_print (&(pState->name[0]));
}

static void defaultEventHomeStop (HomeState *pState)
{
    rob_print_from_program_space (PSTR ("!eH- in "));
    rob_print (&(pState->name[0]));
}

/*
 * PUBLIC FUNCTIONS:
 */

void defaultImplementation (HomeState *pState)
{
    pState->pEventHomeStart = defaultEventHomeStart;
    pState->pEventHomeRoughIntegrationDone = defaultEventHomeRoughIntegrationDone;
    pState->pEventHomeRoughAlignmentDone = defaultEventHomeRoughAlignmentDone;
    pState->pEventHomeRoughAlignmentFailed = defaultEventHomeRoughAlignmentFailed;
    pState->pEventHomeFineIntegrationDone = defaultEventHomeFineIntegrationDone;
    pState->pEventHomeFineAlignmentDone = defaultEventHomeFineAlignmentDone;
    pState->pEventHomeFineAlignmentFailed = defaultEventHomeFineAlignmentFailed;
    pState->pEventHomeTravelIntegrationDone = defaultEventHomeTravelIntegrationDone;
    pState->pEventHomeTravelAlignmentFailed = defaultEventHomeTravelAlignmentFailed;
    pState->pEventHomeStop = defaultEventHomeStop;
}