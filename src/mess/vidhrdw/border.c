/***************************************************************************
        Border engine:

        Functions for drawing multi-coloured screen borders using the
        Event List processing.

Changes:

28/05/2000 DJR - Initial implementation.

***************************************************************************/

#include "driver.h"
#include "border.h"
#include "mess/eventlst.h"

/* Last border colour output in the previous frame */
static int CurrBorderColor = 0;

static int LastDisplayedBorderColor = -1; /* Negative value indicates redraw */

/* Force the border to be redrawn on the next frame */
void force_border_redraw (void)
{
        LastDisplayedBorderColor = -1;
}

/* Set the last border colour to have been displayed. Used when loading snap
   shots and to record the last colour change in a frame that was skipped. */
void set_last_border_color (int NewColor)
{
        CurrBorderColor = NewColor;
}

void draw_border(struct osd_bitmap *bitmap,
        int TopBorderLines,             /* Border lines before actual screen */
        int ScreenLines,                /* Screen height in pixels */
        int BottomBorderLines,          /* Border lines below screen */
        int LeftBorderPixels,           /* Border pixels to the left of each screen line */
        int ScreenPixels,               /* Width of actual screen in pixels */
        int RightBorderPixels,          /* Border pixels to the right of each screen line */
        int LeftBorderCycles,           /* Cycles taken to draw left border of each scan line */
        int ScreenCycles,               /* Cycles taken to draw screen data part of each scan line */
        int RightBorderCycles,          /* Cycles taken to draw right border of each scan line */
        int HorizontalRetraceCycles,    /* Cycles taken to return to LHS of CRT after each scan line */
        int VRetraceTime)               /* Cycles taken before start of first border line */
{
        EVENT_LIST_ITEM *pItem;
        int TotalScreenHeight = TopBorderLines+ScreenLines+BottomBorderLines;
        int TotalScreenWidth = LeftBorderPixels+ScreenPixels+RightBorderPixels;
        int DisplayCyclesPerLine = LeftBorderCycles+ScreenCycles+RightBorderCycles;
        int CyclesPerLine = DisplayCyclesPerLine+HorizontalRetraceCycles;
        int CyclesSoFar = 0, NumItems, CurrItem = 0;
        int Count, ScrX, NextScrX, ScrY;
        struct rectangle r;

        pItem = EventList_GetFirstItem();
        NumItems = EventList_NumEvents();

        if (NumItems)
        {
                int CyclesPerFrame = (int)(Machine->drv->cpu[0].cpu_clock / Machine->drv->frames_per_second);
                logerror ("Event count = %d, curr cycle = %ld, total cycles = %ld \n", NumItems, cpu_getcurrentcycles(), CyclesPerFrame);
        }
        for (Count = 0; Count < NumItems; Count++)
                logerror ("Event no %05d, ID = %04x, data = %04x, time = %ld\n", Count, pItem[Count].Event_ID, pItem[Count].Event_Data, pItem[Count].Event_Time);


        if (NumItems == 1)
                CurrBorderColor = pItem->Event_Data;

        if ((NumItems < 2) && (CurrBorderColor==LastDisplayedBorderColor))
        {
                /* Do nothing if border colour has not changed */
        }
        else if (NumItems < 2)
        {
                /* Single border colour - this is not strictly correct as the
                   colour change may have occurred midway through the frame
                   or after the last visible border line however the whole
                   border would be redrawn in the correct colour during the
                   next frame anyway! */
                r.min_x = 0;
                r.max_x = TotalScreenWidth-1;
                r.min_y = 0;
                r.max_y = TopBorderLines-1;
                fillbitmap(bitmap, Machine->pens[CurrBorderColor], &r);

                r.min_x = 0;
                r.max_x = LeftBorderPixels-1;
                r.min_y = TopBorderLines;
                r.max_y = TopBorderLines+ScreenLines-1;
                fillbitmap(bitmap, Machine->pens[CurrBorderColor], &r);

                r.min_x = LeftBorderPixels+ScreenPixels;
                r.max_x = TotalScreenWidth-1;
                fillbitmap(bitmap, Machine->pens[CurrBorderColor], &r);

                r.min_x = 0;
                r.max_x = TotalScreenWidth-1;
                r.min_y = TopBorderLines+ScreenLines;
                r.max_y = TotalScreenHeight-1;
                fillbitmap(bitmap, Machine->pens[CurrBorderColor], &r);
                LastDisplayedBorderColor = CurrBorderColor;

                logerror ("Setting border colour to %d\n", CurrBorderColor);
        }
        else
        {
                /* Multiple border colours */

                /* Process entries before first displayed line */
                while ((CurrItem < NumItems) && (pItem[CurrItem].Event_Time <= VRetraceTime))
                {
                        CurrBorderColor = pItem[CurrItem].Event_Data;
                        CurrItem++;
                }

                /* Draw top border */
                CyclesSoFar = VRetraceTime;
                for (ScrY = 0; ScrY < TopBorderLines; ScrY++)
                {
                        r.min_x = 0;
                        r.min_y = r.max_y = ScrY;
                        if ((CurrItem >= NumItems) || (pItem[CurrItem].Event_Time >= (CyclesSoFar+DisplayCyclesPerLine)))
                        {
                                /* Single colour on line */
                                r.max_x = TotalScreenWidth-1;
                                fillbitmap(bitmap, Machine->pens[CurrBorderColor], &r);
                        }
                        else
                        {
                                /* Multiple colours on a line */
                                ScrX = (int)(pItem[CurrItem].Event_Time - CyclesSoFar) * (float)TotalScreenWidth / (float)DisplayCyclesPerLine;
                                r.max_x = ScrX-1;
                                fillbitmap(bitmap, Machine->pens[CurrBorderColor], &r);
                                CurrBorderColor = pItem[CurrItem].Event_Data;
                                CurrItem++;
                                while ((CurrItem < NumItems) && (pItem[CurrItem].Event_Time < (CyclesSoFar+DisplayCyclesPerLine)))
                                {
                                        NextScrX = (int)(pItem[CurrItem].Event_Time - CyclesSoFar) * (float)TotalScreenWidth / (float)DisplayCyclesPerLine;
                                        r.min_x = ScrX;
                                        r.max_x = NextScrX-1;
                                        fillbitmap(bitmap, Machine->pens[CurrBorderColor], &r);
                                        ScrX = NextScrX;
                                        CurrBorderColor = pItem[CurrItem].Event_Data;
                                        CurrItem++;
                                }
                                r.min_x = ScrX;
                                r.max_x = TotalScreenWidth-1;
                                fillbitmap(bitmap, Machine->pens[CurrBorderColor], &r);
                        }

                        /* Process colour changes during horizontal retrace */
                        CyclesSoFar+= CyclesPerLine;
                        while ((CurrItem < NumItems) && (pItem[CurrItem].Event_Time <= CyclesSoFar))
                        {
                                CurrBorderColor = pItem[CurrItem].Event_Data;
                                CurrItem++;
                        }
                }

                /* Draw left and right borders next to screen lines */
                for (ScrY = TopBorderLines; ScrY < (TopBorderLines+ScreenLines); ScrY++)
                {
                        /* Draw left hand border */
                        r.min_x = 0;
                        r.min_y = r.max_y = ScrY;

                        if ((CurrItem >= NumItems) || (pItem[CurrItem].Event_Time >= (CyclesSoFar+LeftBorderCycles)))
                        {
                                /* Single colour */
                                r.max_x = LeftBorderPixels-1;
                                fillbitmap(bitmap, Machine->pens[CurrBorderColor], &r);
                        }
                        else
                        {
                                /* Multiple colours */
                                ScrX = (int)(pItem[CurrItem].Event_Time - CyclesSoFar) * (float)LeftBorderPixels / (float)LeftBorderCycles;
                                r.max_x = ScrX-1;
                                fillbitmap(bitmap, Machine->pens[CurrBorderColor], &r);
                                CurrBorderColor = pItem[CurrItem].Event_Data;
                                CurrItem++;
                                while ((CurrItem < NumItems) && (pItem[CurrItem].Event_Time < (CyclesSoFar+LeftBorderCycles)))
                                {
                                        NextScrX = (int)(pItem[CurrItem].Event_Time - CyclesSoFar) * (float)LeftBorderPixels / (float)LeftBorderCycles;
                                        r.min_x = ScrX;
                                        r.max_x = NextScrX-1;
                                        fillbitmap(bitmap, Machine->pens[CurrBorderColor], &r);
                                        ScrX = NextScrX;
                                        CurrBorderColor = pItem[CurrItem].Event_Data;
                                        CurrItem++;
                                }
                                r.min_x = ScrX;
                                r.max_x = LeftBorderPixels-1;
                                fillbitmap(bitmap, Machine->pens[CurrBorderColor], &r);
                        }

                        /* Process colour changes during screen draw */
                        while ((CurrItem < NumItems) && (pItem[CurrItem].Event_Time <= (CyclesSoFar+LeftBorderCycles+ScreenCycles)))
                        {
                                CurrBorderColor = pItem[CurrItem].Event_Data;
                                CurrItem++;
                        }

                        /* Draw right hand border */
                        r.min_x = LeftBorderPixels+ScreenPixels;
                        if ((CurrItem >= NumItems) || (pItem[CurrItem].Event_Time >= (CyclesSoFar+DisplayCyclesPerLine)))
                        {
                                /* Single colour */
                                r.max_x = TotalScreenWidth-1;
                                fillbitmap(bitmap, Machine->pens[CurrBorderColor], &r);
                        }
                        else
                        {
                                /* Multiple colours */
                                ScrX = LeftBorderPixels + ScreenPixels + (int)(pItem[CurrItem].Event_Time - CyclesSoFar) * (float)RightBorderPixels / (float)RightBorderCycles;
                                r.max_x = ScrX-1;
                                fillbitmap(bitmap, Machine->pens[CurrBorderColor], &r);
                                CurrBorderColor = pItem[CurrItem].Event_Data;
                                CurrItem++;
                                while ((CurrItem < NumItems) && (pItem[CurrItem].Event_Time < (CyclesSoFar+DisplayCyclesPerLine)))
                                {
                                        NextScrX = LeftBorderPixels + ScreenPixels + (int)(pItem[CurrItem].Event_Time - CyclesSoFar) * (float)RightBorderPixels / (float)RightBorderCycles;
                                        r.min_x = ScrX;
                                        r.max_x = NextScrX-1;
                                        fillbitmap(bitmap, Machine->pens[CurrBorderColor], &r);
                                        ScrX = NextScrX;
                                        CurrBorderColor = pItem[CurrItem].Event_Data;
                                        CurrItem++;
                                }
                                r.min_x = ScrX;
                                r.max_x = TotalScreenWidth-1;
                                fillbitmap(bitmap, Machine->pens[CurrBorderColor], &r);
                        }

                        /* Process colour changes during horizontal retrace */
                        CyclesSoFar+= CyclesPerLine;
                        while ((CurrItem < NumItems) && (pItem[CurrItem].Event_Time <= CyclesSoFar))
                        {
                                CurrBorderColor = pItem[CurrItem].Event_Data;
                                CurrItem++;
                        }
                }

                /* Draw bottom border */
                for (ScrY = TopBorderLines+ScreenLines; ScrY < TotalScreenHeight; ScrY++)
                {
                        r.min_x = 0;
                        r.min_y = r.max_y = ScrY;
                        if ((CurrItem >= NumItems) || (pItem[CurrItem].Event_Time >= (CyclesSoFar+DisplayCyclesPerLine)))
                        {
                                /* Single colour on line */
                                r.max_x = TotalScreenWidth-1;
                                fillbitmap(bitmap, Machine->pens[CurrBorderColor], &r);
                        }
                        else
                        {
                                /* Multiple colours on a line */
                                ScrX = (int)(pItem[CurrItem].Event_Time - CyclesSoFar) * (float)TotalScreenWidth / (float)DisplayCyclesPerLine;
                                r.max_x = ScrX-1;
                                fillbitmap(bitmap, Machine->pens[CurrBorderColor], &r);
                                CurrBorderColor = pItem[CurrItem].Event_Data;
                                CurrItem++;
                                while ((CurrItem < NumItems) && (pItem[CurrItem].Event_Time < (CyclesSoFar+DisplayCyclesPerLine)))
                                {
                                        NextScrX = (int)(pItem[CurrItem].Event_Time - CyclesSoFar) * (float)TotalScreenWidth / (float)DisplayCyclesPerLine;
                                        r.min_x = ScrX;
                                        r.max_x = NextScrX-1;
                                        fillbitmap(bitmap, Machine->pens[CurrBorderColor], &r);
                                        ScrX = NextScrX;
                                        CurrBorderColor = pItem[CurrItem].Event_Data;
                                        CurrItem++;
                                }
                                r.min_x = ScrX;
                                r.max_x = TotalScreenWidth-1;
                                fillbitmap(bitmap, Machine->pens[CurrBorderColor], &r);
                        }

                        /* Process colour changes during horizontal retrace */
                        CyclesSoFar+= CyclesPerLine;
                        while ((CurrItem < NumItems) && (pItem[CurrItem].Event_Time <= CyclesSoFar))
                        {
                                CurrBorderColor = pItem[CurrItem].Event_Data;
                                CurrItem++;
                        }
                }

                /* Process colour changes after last displayed line */
                CurrBorderColor = pItem[NumItems-1].Event_Data;

                /* Set value to ensure redraw on next frame */
                LastDisplayedBorderColor = -1;

                logerror ("Multi coloured border drawn\n", CurrBorderColor);
        }
        EventList_Reset();
        EventList_SetOffsetStartTime ( cpu_getcurrentcycles() );
}
