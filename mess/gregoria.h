// gregorian calendar

#ifdef __cplusplus
extern "C" {
#endif

	int	gregorian_is_leap_year(int year);

	// month 1..12 !
	int gregorian_days_in_month(int month, int year);

#ifdef __cplusplus
}
#endif
