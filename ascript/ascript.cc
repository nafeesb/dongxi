/*! \file ascript.cc
  \author Nafees Bin Zafar <nafees@nafees.net>
  \date 10/10/03

  $Id: ascript.cc,v 1.1.1.1 2003/11/11 06:32:39 nafees Exp $

  \brief This program is a wrapper around osascript to allow for
  unix styled AppleScript shell scripts.  This will allow you to write
  a shell script beginning with
    #!/path/to/ascript
  followed by the AppleScript source.

  Here is a dinky example:

#!/usr/bin/ascript

--  Created by Nafees Bin Zafar on Mon Nov 10 2003.

tell application "iTunes"
	if player state is not playing then
		play
	else
		pause
	end if
end tell

*/

/*
    ascript: A wrapper for Apple's osascript utility
    Copyright (C) 2003  Nafees Bin Zafar

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU Lesser General Public
    License as published by the Free Software Foundation; either
    version 2.1 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public
    License along with this library; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
  
*/

#include <iostream>
#include <fstream>
using namespace std;

#include <stdio.h>

int main(int argc, char** argv) {
  ifstream script_in;

  /*!
    \note Expect that if we are passed the script name as an argument
    then we need to remove the very first line from it.  This line
    ought to contain #!/path/to/ascript.  If we are not given a
    commandline argument then we read from stdin.  In this case we do
    not expect a #!... string as the first line.    
  */
  if (argc == 1)
    script_in.open("-");
  else {
    script_in.open(argv[1]);
    string tmp;
    getline(script_in, tmp);
  }

  FILE* interp = popen ("/usr/bin/osascript", "w");

  char buf[1024];
  while (script_in.read(buf, sizeof(buf)) || script_in.gcount()) {
    buf[script_in.gcount()] = 0;
    fprintf (interp, "%s", buf);
  }
  
  pclose(interp);
  script_in.close();

  return 0;
}
