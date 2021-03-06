<?php
//	Copyright (C) 2012 Mark Vejvoda, Titus Tscharntke and Tom Reynolds
//	The Megaglest Team, under GNU GPL v3.0
// ==============================================================

	if ( !defined('INCLUSION_PERMITTED') || ( defined('INCLUSION_PERMITTED') && INCLUSION_PERMITTED !== true ) ) { die( 'This file must not be invoked directly.' ); }

	define( 'PRODUCT_NAME',       'MegaGlest' );
	define( 'PRODUCT_URL',        'http://megaglest.org' );

	define( 'MYSQL_HOST',         '127.0.0.1' );
	define( 'MYSQL_DATABASE',     'glest' );
	define( 'MYSQL_USER',         'root' );
	define( 'MYSQL_PASSWORD',     'your_pwd' );

	// Allow using persistent MYSQL database server connections
	// This _can_ improve performance remarkably but _can_ also break stuff completely, so read up:
	// http://php.net/manual/function.mysql-pconnect.php
	// http://php.net/manual/features.persistent-connections.php
	define( 'MYSQL_LINK_PERSIST', false );

	// How many recently seen servers to store
	define( 'MAX_RECENT_SERVERS', 5 );

	define( 'DEFAULT_COUNTRY',    '??' );
?>
