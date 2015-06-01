/*
 * State machine, based on "Patterns in C - Part 2: STATE" by Adam Petersen
 * http://www.adampetersen.se/Patterns%20in%20C%202,%20STATE.pdf
 *
 * This application uses the Pololu AVR C/C++ Library.  For help, see:
 * -User's guide: http://www.pololu.com/docs/0J20
 * -Command reference: http://www.pololu.com/docs/0J18
 */

/*
 * MANIFEST CONSTANTS
 */
#define STATE_NAME_STRING_LENGTH 10

/*
 * GENERAL TYPES
 */
typedef struct HomeStateTag
{
    unsigned int countRoughAlignmentEntries;
    unsigned int countFineAlignmentEntries;
    unsigned int countTravelEntries;
    char name[STATE_NAME_STRING_LENGTH];
    void (*pEventHomeStart) (struct HomeStateTag *pState);
    void (*pEventHomeRoughIntegrationDone) (struct HomeStateTag *pState);
    void (*pEventHomeRoughAlignmentDone) (struct HomeStateTag *pState);
    void (*pEventHomeRoughAlignmentFailed) (struct HomeStateTag *pState);
    void (*pEventHomeFineIntegrationDone) (struct HomeStateTag *pState);
    void (*pEventHomeFineAlignmentDone) (struct HomeStateTag *pState);
    void (*pEventHomeFineAlignmentFailed) (struct HomeStateTag *pState);
    void (*pEventHomeTravelIntegrationDone) (struct HomeStateTag *pState);
    void (*pEventHomeTravelAlignmentFailed) (struct HomeStateTag *pState);
    void (*pEventHomeStop) (struct HomeStateTag *pState);
} HomeState;

typedef struct HomeContextTag
{
  HomeState state;
} HomeContext;

/*
 * FUNCTION PROTOTYPES
 */

void defaultImplementation (HomeState *pState);