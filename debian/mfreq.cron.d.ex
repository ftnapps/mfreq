#
# Regular cron jobs for the mfreq package
#
0 4	* * *	root	[ -x /usr/bin/mfreq_maintenance ] && /usr/bin/mfreq_maintenance
