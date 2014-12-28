Titanium
========

Titanium, A small linux system information collector based on adkit_. It uses a UDP protocol to receive system
information and then collects it and presents it as a web page for statistical analysis.
The information is also stored in a MySQL database so other applications may use it.

.. _adkit: https://github.com/Justasic/adkit


Setup
-----

Right now Titanium only supports linux but will soon support all platforms.

To setup, clone the repository and compile both the master branch and
the client branch.

Copy the client binary to all your servers (provided the architecture is
supported) and run it with the connection details provided over command line.

On the server, Just simply edit the configuration, create a database with
the mysqlschema.sql file and run it.

You will also need to setup a FastCGI-compatible web server.

Once all of this is setup, you should be able to visit the website and see
plenty of information flowing into the page.
