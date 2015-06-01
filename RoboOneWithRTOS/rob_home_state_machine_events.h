/*
 * State machine, based on "Patterns in C - Part 2: STATE" by Adam Petersen
 * http://www.adampetersen.se/Patterns%20in%20C%202,%20STATE.pdf
 */

/*
 * MANIFEST CONSTANTS
 */

/*
 * TYPES
 */

/*
 * FUNCTION PROTOTYPES
 */
void eventHomeStartOrangutan (HomeContext *pInstance);
void eventHomeRoughIntegrationDoneOrangutan (HomeContext *pInstance);
void eventHomeRoughAlignmentDoneOrangutan (HomeContext *pInstance);
void eventHomeRoughAlignmentFailedOrangutan (HomeContext *pInstance);
void eventHomeFineIntegrationDoneOrangutan (HomeContext *pInstance);
void eventHomeFineAlignmentDoneOrangutan (HomeContext *pInstance);
void eventHomeFineAlignmentFailedOrangutan (HomeContext *pInstance);
void eventHomeTravelIntegrationDoneOrangutan (HomeContext *pInstance);
void eventHomeTravelAlignmentFailedOrangutan (HomeContext *pInstance);
void eventHomeDoneOrangutan (HomeContext *pInstance);
void eventHomeStopOrangutan (HomeContext *pInstance);