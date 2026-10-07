#ifndef NOTY_RESOURCE_H
#define NOTY_RESOURCE_H

/*
 * Resource IDs shared between the .rc script and the C/C++ runtime.
 * This header must remain valid C.
 */

/* RCDATA blobs ----------------------------------------------------------- */
#define IDR_LOGO_PNG            101

/* Reserved font slots (filled in a later phase when Rubik is provided). */
#define IDR_FONT_RUBIK_REGULAR  201
#define IDR_FONT_RUBIK_MEDIUM   202
#define IDR_FONT_RUBIK_SEMIBOLD 203
#define IDR_FONT_RUBIK_BOLD     204
#define IDR_FONT_RUBIK_BLACK    205

/* Icons ------------------------------------------------------------------ */
#define IDI_NOTY_APP            1000

#endif /* NOTY_RESOURCE_H */