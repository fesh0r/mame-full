#ifdef __cplusplus
extern "C" {
#endif

// call this at init time to add your state functions
void state_add_function(void (*function)(void));

// call this in your state function to output text
void state_display_text(const char *text);

// call this at last after updating your frame
void state_display(struct osd_bitmap *bitmap);


#ifdef __cplusplus
}
#endif
