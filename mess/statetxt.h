#ifdef __cplusplus
extern "C" {
#endif

// call this at init time to add your state functions
void statetext_add_function(void (*function)(void));

// call this in your state function to output text
void statetext_display_text(const char *text);

// call this at last after updating your frame
void statetext_display(struct mame_bitmap *bitmap);


#ifdef __cplusplus
}
#endif
