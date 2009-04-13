<?php
	#<!-- setcookie -->
		$pageCount = $_COOKIE["pageCount"];
		setcookie("pageCount", ++$pageCount);

	#<!-- sessions -->

		session_start();
		session_register("hits");
		$_SESSION["hits"]++;
		$sessionId = $_COOKIE["PHPSESSID"];

	# After header setup

		echo "<p>Session ID $sessionId</p>";
		$hits = $_SESSION["hits"];
		echo "<p>Session page count: $hits</p>";
		echo "<p>Cookie page count:  $pageCount</p>";

	#<!-- Include -->
		include 'include.php';

	#<!-- substrings -->
		$sub = substr("abcdef", 2, 3);
		echo "<p>Result of substring is \"$sub\"</p>";

	#<!-- regexp -->
		ereg('^cow', 'Dave was a cowhand');
		preg_match('#/usr/local#', '/usr/local/bin/perl');

	#<!-- mysql -->
	#	$handle = mysql_connect("localhost", "guest", "");

	#<!-- XML -->
		$parser = xml_parser_create();

	#<!-- XML-RPC -->
		$xmlRpc = xmlrpc_server_create();

?>

<html>
<head>

</head>


<body>
	<p>&nbsp;</p>
	Hello World Name 
	<form name="details" method="post" action="features.php">
		Name <input type="text" name="name" 
			value="<? echo $HTTP_POST_VARS[name] ?>">
		Address <input type="text" name="address" 
			value="<? echo $HTTP_POST_VARS[address] ?>">
		<input type="submit" name="submit" value="OK">
	</form>
</body>
</html>
