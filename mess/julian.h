// julian calendar

#ifdef __cplusplus
extern "C" {
#endif

	int	julian_is_leap_year(int year);

	// month 1..12 !
	int julian_days_in_month(int month, int year);

#ifdef __cplusplus
}
#endif
