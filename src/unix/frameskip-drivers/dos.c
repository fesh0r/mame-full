#include "xmame.h"
#include "driver.h"
#include "profiler.h"
#include "sysdep/misc.h"

static int frameskip_counter = 0;

int dos_skip_next_frame(int show_fps_counter, struct mame_bitmap *bitmap)
{
	static const int skiptable[FRAMESKIP_LEVELS][FRAMESKIP_LEVELS] =
	{
		{ 0,0,0,0,0,0,0,0,0,0,0,0 },
		{ 0,0,0,0,0,0,0,0,0,0,0,1 },
		{ 0,0,0,0,0,1,0,0,0,0,0,1 },
		{ 0,0,0,1,0,0,0,1,0,0,0,1 },
		{ 0,0,1,0,0,1,0,0,1,0,0,1 },
		{ 0,1,0,0,1,0,1,0,0,1,0,1 },
		{ 0,1,0,1,0,1,0,1,0,1,0,1 },
		{ 0,1,0,1,1,0,1,0,1,1,0,1 },
		{ 0,1,1,0,1,1,0,1,1,0,1,1 },
		{ 0,1,1,1,0,1,1,1,0,1,1,1 },
		{ 0,1,1,1,1,1,0,1,1,1,1,1 },
		{ 0,1,1,1,1,1,1,1,1,1,1,1 }
	};
	static const int waittable[FRAMESKIP_LEVELS][FRAMESKIP_LEVELS] =
	{
		{ 1,1,1,1,1,1,1,1,1,1,1,1 },
		{ 2,1,1,1,1,1,1,1,1,1,1,0 },
		{ 2,1,1,1,1,0,2,1,1,1,1,0 },
		{ 2,1,1,0,2,1,1,0,2,1,1,0 },
		{ 2,1,0,2,1,0,2,1,0,2,1,0 },
		{ 2,0,2,1,0,2,0,2,1,0,2,0 },
		{ 2,0,2,0,2,0,2,0,2,0,2,0 },
		{ 2,0,2,0,0,3,0,2,0,0,3,0 },
		{ 3,0,0,3,0,0,3,0,0,3,0,0 },
		{ 4,0,0,0,4,0,0,0,4,0,0,0 },
		{ 6,0,0,0,0,0,6,0,0,0,0,0 },
		{12,0,0,0,0,0,0,0,0,0,0,0 }
	};
	int i;
	uclock_t curr;
	static uclock_t prev_frames[FRAMESKIP_LEVELS]={0,0,0,0,0,0,0,0,0,0,0,0};
	static uclock_t prev=0;
	static int speed=100;

	/* now wait until it's time to update the screen */
	if (skiptable[frameskip][frameskip_counter] == 0)
	{
        	if (throttle)
        	{
        		uclock_t target,target2;
        		profiler_mark(PROFILER_IDLE);

        		/* wait until enough time has passed since last frame... */
        		target = prev +
        			waittable[frameskip][frameskip_counter] * UCLOCKS_PER_SEC/video_fps;

        		/* ... OR since FRAMESKIP_LEVELS frames ago. This way, if a frame takes */
        		/* longer than the allotted time, we can compensate in the following frames. */
        		target2 = prev_frames[frameskip_counter] +
        			FRAMESKIP_LEVELS * UCLOCKS_PER_SEC/video_fps;

        		if (target - target2 > 0) target = target2;
        		
        		curr = uclock();
        		
        		/* If we need to sleep more then half a second,
        		   we've somehow got totally out of sync. So
        		   if this happens we reset all counters */
        		if ((target - curr) > (UCLOCKS_PER_SEC / 2))
        		   for (i=0; i < FRAMESKIP_LEVELS; i++)
        		      prev_frames[i] = curr;
        		else
        		   while ((curr - target) < 0)
        		   {
        		      curr = uclock();
        		      if ((target - curr) > (UCLOCKS_PER_SEC / 1000) &&
        		          sleep_idle)
        		         usleep(100);
        		   }

        		profiler_mark(PROFILER_END);
        	}
        	else curr = uclock();

        	if (frameskip_counter == 0 && (curr - prev_frames[frameskip_counter]))
        	{
        		int divdr;

        		divdr = video_fps * (curr - prev_frames[frameskip_counter]) / (100 * FRAMESKIP_LEVELS);
        		speed = (UCLOCKS_PER_SEC + divdr/2) / divdr;
        	}

        	prev = curr;
        	for (i = 0;i < waittable[frameskip][frameskip_counter];i++)
        		prev_frames[(frameskip_counter + FRAMESKIP_LEVELS - i) % FRAMESKIP_LEVELS] = curr;

        	if (show_fps_counter)
        	{
        		int fps;
        		char buf[30];
        		int divdr;

        		divdr = 100 * FRAMESKIP_LEVELS;
        		fps = (video_fps * (FRAMESKIP_LEVELS - frameskip) * speed + (divdr / 2)) / divdr;
        		sprintf(buf,"%s%s%s%2d %3d%%(%2d/%d fps)", throttle?"T ":"",
			(throttle && sleep_idle)?"S ":"",
			(throttle && autoframeskip)?"autofskp":"fskp",
			frameskip,speed,fps,(int)video_fps);
        		ui_text(bitmap, buf, Machine->uiwidth-strlen(buf)*Machine->uifontwidth, 0);
        	}
        	
		if (throttle && autoframeskip && frameskip_counter == 0)
		{
			static int frameskipadjust;

			if (speed >= 100)
			{
				frameskipadjust++;
				if (frameskipadjust >= 3)
				{
					frameskipadjust = 0;
					if (frameskip > 0) frameskip--;
				}
			}
			else
			{
				if (speed < 80)
					frameskipadjust -= (90 - speed) / 5;
				else
				{
					/* don't push frameskip too far if we are close to 100% speed */
					if (frameskip < 8)
						frameskipadjust--;
				}

				while (frameskipadjust <= -2)
				{
					frameskipadjust += 2;
					if (frameskip < max_autoframeskip) frameskip++;
				}
			}
		}
	}
	
	frameskip_counter = (frameskip_counter + 1) % FRAMESKIP_LEVELS;
	
	return skiptable[frameskip][frameskip_counter];
}
