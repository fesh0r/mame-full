#include "driver.h"

extern int CurrentVolume;

int     osd_handle_event(void)
{
static  int showvoltemp = 0;

        /* if the user pressed ESC, stop the emulation */
        if (osd_key_pressed(UI_KEY_ESCAPE))
                return 1;

        if (osd_key_pressed(UI_KEY_DEC_VOLUME) && osd_key_pressed(OSD_KEY_LSHIFT) == 0)
        {
                /* decrease volume */
                if (CurrentVolume > 0) CurrentVolume--;
                osd_set_mastervolume(CurrentVolume);
                showvoltemp = 50;
        }

        if (osd_key_pressed(UI_KEY_INC_VOLUME) && osd_key_pressed(OSD_KEY_LSHIFT) == 0)
        {
                /* increase volume */
                if (CurrentVolume < 100) CurrentVolume++;
                osd_set_mastervolume(CurrentVolume);
                showvoltemp = 50;
        }                                          /* MAURY_END: new options */

        if (osd_key_pressed(UI_KEY_PAUSE)) /* pause the game */
        {
                struct DisplayText dt[2];
                int count = 0;


                dt[0].text = "PAUSED";
                dt[0].color = DT_COLOR_RED;
                dt[0].x = (Machine->uiwidth - Machine->uifont->width * strlen(dt[0].text)) / 2;
                dt[0].y = (Machine->uiheight - Machine->uifont->height) / 2;
                dt[1].text = 0;

                osd_set_mastervolume(0);

                while (osd_key_pressed(UI_KEY_PAUSE))
                        osd_update_audio();     /* give time to the sound hardware to apply the volume change */

                while (osd_key_pressed(UI_KEY_PAUSE) == 0 && osd_key_pressed(UI_KEY_ESCAPE) == 0)
                {
                        if (osd_key_pressed(UI_KEY_MENU)) setup_menu(); /* call the configuration menu */

                        osd_clearbitmap(Machine->scrbitmap);

                        (*Machine->drv->vh_update)(Machine->scrbitmap);  /* redraw screen */

                        if (count < Machine->drv->frames_per_second / 2)
                                displaytext(dt,0);      /* make PAUSED blink */
                        else
                                osd_update_display();

                        count = (count + 1) % (Machine->drv->frames_per_second / 1);
                }

                while (osd_key_pressed(UI_KEY_ESCAPE));   /* wait for jey release */
                while (osd_key_pressed(UI_KEY_PAUSE));     /* ditto */

                osd_set_mastervolume(CurrentVolume);
                osd_clearbitmap(Machine->scrbitmap);
        }

        /* if the user pressed TAB, go to the setup menu */
        if (osd_key_pressed(UI_KEY_MENU))
        {
                osd_set_mastervolume(0);

                while (osd_key_pressed(UI_KEY_MENU))
                        osd_update_audio();     /* give time to the sound hardware to apply the volume change */

                if (setup_menu()) return 1;

                osd_set_mastervolume(CurrentVolume);
        }

        /* if the user pressed F4, show the character set */
        if (osd_key_pressed(UI_KEY_CHARSET))
        {
                osd_set_mastervolume(0);

                while (osd_key_pressed(UI_KEY_CHARSET))
                        osd_update_audio();     /* give time to the sound hardware to apply the volume change */

                if (showcharset()) return 1;

                osd_set_mastervolume(CurrentVolume);
        }

        if (showvoltemp)
        {
                showvoltemp--;
                if (!showvoltemp)
                {
                        osd_clearbitmap(Machine->scrbitmap);
                }
                else
                {                     /* volume-meter */
                int trueorientation;
                int i,x;
                char volstr[25];
                        trueorientation = Machine->orientation;
                        Machine->orientation = ORIENTATION_DEFAULT;

                        x = (Machine->uiwidth - 24*Machine->uifont->width)/2;
                        strcpy(volstr,"                      ");
                        for (i = 0;i < (CurrentVolume/5);i++) volstr[i+1] = '\x15';

                        drawgfx(Machine->scrbitmap,Machine->uifont,16,DT_COLOR_RED,0,0,x,Machine->drv->screen_height/2,0,TRANSPARENCY_NONE,0);
                        drawgfx(Machine->scrbitmap,Machine->uifont,17,DT_COLOR_RED,0,0,x+23*Machine->uifont->width,Machine->drv->screen_height/2,0,TRANSPARENCY_NONE,0);
                        for (i = 0;i < 22;i++)
                            drawgfx(Machine->scrbitmap,Machine->uifont,(unsigned int)volstr[i],DT_COLOR_WHITE,
                                        0,0,x+(i+1)*Machine->uifont->width+Machine->uixmin,Machine->uiheight/2+Machine->uiymin,0,TRANSPARENCY_NONE,0);

                        Machine->orientation = trueorientation;
                }
        }

        return 0;
}
