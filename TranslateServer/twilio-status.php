<?php
echo 'ASD' . PHP_EOL;
$ErrorCode=0;
$StatusCode=0;
$MessageSid = $_REQUEST["MessageSid"];
$MessageStatus = $_POST["MessageStatus"];
if (in_array("ErrorCode", $_POST))
	$ErrorCode = $_POST["ErrorCode"];
$ErrorMessage = $_POST["ErrorMessage"];
if ($MessageStatus == "unknown")
	$StatusCode=0;
else if ($MessageStatus == "queued")
	$StatusCode=1;
else if ($MessageStatus == "sending")
	$StatusCode=2;
else if ($MessageStatus == "sent")
	$StatusCode=3;
else if ($MessageStatus == "delivered")
	$StatusCode=4;
else if ($MessageStatus == "undelivered")
	$StatusCode=-1;
else if ($MessageStatus == "failed")
	$StatusCode=-2;

$servername = "localhost";
$username= "roman";
$password = "";
$dbname = "Translate";	

$sql = "UPDATE sms_twilio SET status=" . $StatusCode . ", error=" . $ErrorCode . " WHERE id=\"" . $MessageSid. "\"";
$conn = new mysqli($servername, $username, $password, $dbname);
if ($conn->connection_error)
	die("Connection to MySQL failed: ". $conn->connection_error);
$conn->query($sql);

$str = "Time:" . date('Y-m-d H:i:s') . PHP_EOL;
$str .= "MessageSid=" . $MessageSid . PHP_EOL;
$str .= "MessageStatus=" . $MessageStatus . PHP_EOL;
if ($MessageStatus == "undelivered") {
	$str .= "ErrorCode=" . $ErrorCode . PHP_EOL;
	$str .= "ErrorMessage=" . $ErrorMessage . PHP_EOL;
}
$str .= PHP_EOL;
$file = 'twilio/status.log';
file_put_contents($file, $str, FILE_APPEND | LOCK_EX);
?>
