/* MAX_NUMBER is the next number to be used, always one more than the highest message number. */
set bulk_insert INSERT INTO FACILITIES (LAST_CHANGE, FACILITY, FAC_CODE, MAX_NUMBER) VALUES (?, ?, ?, ?);
--
('2007-01-17 12:24:00', 'JRD', 0, 561)
('2005-09-02 00:55:59', 'QLI', 1, 513)
('1996-11-07 13:38:37', 'GDEF', 2, 345)
('2005-07-20 04:04:04', 'GFIX', 3, 115)
('1996-11-07 13:39:40', 'GPRE', 4, 1)
--
--('1996-11-07 13:39:40', 'GLTJ', 5, 1)
--('1996-11-07 13:39:40', 'GRST', 6, 1)
--
('2005-11-05 13:09:00', 'DSQL', 7, 32)
('2007-03-22 06:41:07', 'DYN', 8, 246)
--
--('1996-11-07 13:39:40', 'FRED', 9, 1)
--
('1996-11-07 13:39:40', 'INSTALL', 10, 1)
('1996-11-07 13:38:41', 'TEST', 11, 4)
('2006-11-05 23:19:03', 'GBAK', 12, 295)
('2006-11-05 11:20:00', 'SQLERR', 13, 947)
('1996-11-07 13:38:42', 'SQLWARN', 14, 613)
('2006-09-10 03:04:31', 'JRD_BUGCHK', 15, 307)
--
--('1996-11-07 13:38:43', 'GJRN', 16, 241)
--
('2007-02-25 08:33:07', 'ISQL', 17, 157)
('1998-11-04 11:06:15', 'GSEC', 18, 91)
('2002-03-05 02:30:12', 'LICENSE', 19, 60)
('2002-03-05 02:31:54', 'DOS', 20, 74)
('2001-10-10 18:05:16', 'GSTAT', 21, 36)
stop

COMMIT WORK;

