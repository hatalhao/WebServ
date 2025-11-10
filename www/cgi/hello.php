<?php
// In PHP CLI mode (used for CGI), header() doesn't output to stdout
// So we must manually echo headers for CGI
echo "Content-Type: text/plain\n";
echo "\n";

echo " PHP Test Successful\n";
echo "--------------------------\n\n";

echo "PHP Version: " . phpversion() . "\n";
echo "Server API: " . php_sapi_name() . "\n";
echo "Running as user: " . get_current_user() . "\n";
?>