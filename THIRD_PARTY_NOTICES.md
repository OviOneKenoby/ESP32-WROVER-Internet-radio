# Third-party notices

This file lists third-party material that is contained in this repository or
that is built into the firmware image, with the licence information found in
the upstream files. Copyright lines are quoted as they appear in the upstream
files. The licence texts at the end are copied from the upstream files named
under each text (line endings normalised to LF). The project's own code is
not covered by this file.

## 1. Third-party files contained in this repository

All of these come from ESP8266Audio 2.2.0 (pinned commit
`058e131b26e459b9aadcb589a50f07877f1a09fd`), https://github.com/earlephilhower/ESP8266Audio,
by Earle F. Philhower, III. The upstream files carry the header
"Copyright (C) 2017  Earle F. Philhower, III" and the GNU General Public License,
version 3 or (at your option) any later version (see licence text A below).

| File | Origin | Note |
|---|---|---|
| `src/AudioFileSourceHTTPSStream.h`, `src/AudioFileSourceHTTPSStream.cpp` | `AudioFileSourceHTTPStream` | uses a secure client; header states "Copyright (C) 2017 Earle F. Philhower, III, GPLv3" |
| `src/AudioFileSourceICYSStream.h`, `src/AudioFileSourceICYSStream.cpp` | `AudioFileSourceICYStream` | based on the HTTPS variant above; the metadata block is read by `src/icy_metadata.h` |
| `src/AudioFileSourcePlainICYStream.h`, `src/AudioFileSourcePlainICYStream.cpp` | `AudioFileSourceHTTPStream` and `AudioFileSourceICYStream` merged into one class | the upstream licence header is kept in the `.cpp` |
| `src/icy_metadata.h` | derived from the metadata block handling in `AudioFileSourceICYStream::readInternal` | header names the origin and licence |

## 2. Libraries built into the firmware image (not contained in this repository)

Fetched by PlatformIO as listed in `platformio.ini`. Versions are those of the
verified build.

| Component | Version | Upstream | Licence (as found) | Copyright line (as found) | Used for |
|---|---|---|---|---|---|
| ESP8266Audio | 2.2.0 | https://github.com/earlephilhower/ESP8266Audio | GNU GPL v3 (licence text A) | "Copyright (C) 2017  Earle F. Philhower, III" (file headers) | audio sources, MP3 and AAC decoder glue, buffer, I2S output |
| Helix MP3 / AAC decoders (inside ESP8266Audio: `src/libhelix-mp3`, `src/libhelix-aac`) | as shipped in ESP8266Audio 2.2.0 | https://github.com/earlephilhower/ESP8266Audio | file headers: RealNetworks Public Source License 1.0 (RPSL) or Community Source License 1.0 (RCSL); texts in `libhelix-mp3` (licence texts H, I, J) | "Portions Copyright (c) 1995-2002 RealNetworks, Inc. All Rights Reserved." (`mp3dec.c`); "Portions Copyright (c) 1995-2005 RealNetworks, Inc. All Rights Reserved." (`aacdec.c`); "Copyright (c) 1995-2004 RealNetworks, Inc. All Rights Reserved." (`LICENSE.txt`) | MP3 and AAC decoding |
| GxEPD2 | 1.6.9 | https://github.com/ZinggJM/GxEPD2 | GNU GPL v3 (licence text A; the library ships its own `LICENSE`) | "Author: Jean-Marc Zingg" (file headers) | e-paper display driver |
| Adafruit GFX Library | 1.12.6 | https://github.com/adafruit/Adafruit-GFX-Library | BSD licence (licence text G) | "Copyright (c) 2012 Adafruit Industries.  All rights reserved." (`license.txt`); "Copyright (c) 2013 Adafruit Industries.  All rights reserved." (source headers) | graphics, bitmap fonts (`Fonts/FreeSerif9pt7b.h`, `Fonts/FreeMonoBold9pt7b.h`) |
| Adafruit BusIO | 1.17.4 | https://github.com/adafruit/Adafruit_BusIO | MIT (licence text D) | "Copyright (c) 2017 Adafruit Industries" | bus helpers used by Adafruit GFX |
| ArduinoJson | 7.4.3 | https://github.com/bblanchon/ArduinoJson | MIT (licence text E) | "Copyright © 2014-2026, Benoit BLANCHON" | JSON for the web API and the station browser |
| Bounce2 | 2.72 | https://github.com/thomasfredericks/Bounce2 | MIT (licence text F) | "Copyright (c) 2013 thomasfredericks" | button debouncing |
| ESP32-A2DP | 1.8.11 (commit `3245602afc494f9e62160a0cfb2af864af45a37f`) | https://github.com/pschatzmann/ESP32-A2DP | Apache License 2.0 (licence text C) | "Copyright 2020 Phil Schatzmann"; "Copyright 2015-2016 Espressif Systems (Shanghai) PTE LTD" (source headers) | Bluetooth audio sink |

Note: the PlatformIO library folder also contains Adafruit GFX Library 1.13.0
(same `license.txt`); the build log of the verified build lists 1.12.6 as the
library used.

## 3. Framework, SDK and toolchain runtime built into the firmware image

| Component | Version | Licence information found on disk | Used for |
|---|---|---|---|
| Arduino core for the ESP32 (`framework-arduinoespressif32`) and its libraries (WiFi, WebServer, HTTPClient, WiFiClientSecure, Preferences, ESPmDNS, SPI and others) | package 3.20017.241212 (Arduino-ESP32 2.0.17) | `package.json`: "LGPL-2.1-or-later"; source headers, for example `cores/esp32/Arduino.h`: "Copyright (c) 2005-2013 Arduino Team.  All right reserved." and the GNU Lesser General Public License, version 2.1 or later (licence text B) | board support, networking, storage |
| Espressif ESP-IDF prebuilt libraries inside the Arduino core (`tools/sdk/esp32`) | ESP-IDF v4.4.7 (`versions.txt`) | headers, for example `esp_system.h`: "Copyright 2015-2016 Espressif Systems (Shanghai) PTE LTD" and "Licensed under the Apache License, Version 2.0" (licence text C). The prebuilt libraries include components from other projects (for example lwIP, mbedTLS, wpa_supplicant, Bluetooth controller libraries); their own licence texts are not reproduced in this file | Wi-Fi, TLS, TCP/IP, Bluetooth |
| GCC runtime libraries (libgcc, libstdc++) and newlib C library, from `toolchain-xtensa-esp32` | 8.4.0+2021r2-patch5 | the toolchain ships `share/licenses/gcc/COPYING.RUNTIME` (licence text K), `share/licenses/gcc/COPYING3` and `share/licenses/newlib/COPYING.NEWLIB` (licence text L) | C and C++ runtime |

The firmware images stored under `artifacts/` contain the same kind of linked
components as the build described above.

## 4. Licence texts

Letters are used in the tables above: A GPL v3, B LGPL v2.1, C Apache 2.0, D-F
MIT texts, G BSD text, H-J Helix/RealNetworks texts, K GCC Runtime Library
Exception, L newlib notices.

### Licence text A - GNU General Public License, version 3

Text copied from: ESP8266Audio 2.2.0, file LICENSE

~~~~~~~~~~
                    GNU GENERAL PUBLIC LICENSE
                       Version 3, 29 June 2007

 Copyright (C) 2007 Free Software Foundation, Inc. <http://fsf.org/>
 Everyone is permitted to copy and distribute verbatim copies
 of this license document, but changing it is not allowed.

                            Preamble

  The GNU General Public License is a free, copyleft license for
software and other kinds of works.

  The licenses for most software and other practical works are designed
to take away your freedom to share and change the works.  By contrast,
the GNU General Public License is intended to guarantee your freedom to
share and change all versions of a program--to make sure it remains free
software for all its users.  We, the Free Software Foundation, use the
GNU General Public License for most of our software; it applies also to
any other work released this way by its authors.  You can apply it to
your programs, too.

  When we speak of free software, we are referring to freedom, not
price.  Our General Public Licenses are designed to make sure that you
have the freedom to distribute copies of free software (and charge for
them if you wish), that you receive source code or can get it if you
want it, that you can change the software or use pieces of it in new
free programs, and that you know you can do these things.

  To protect your rights, we need to prevent others from denying you
these rights or asking you to surrender the rights.  Therefore, you have
certain responsibilities if you distribute copies of the software, or if
you modify it: responsibilities to respect the freedom of others.

  For example, if you distribute copies of such a program, whether
gratis or for a fee, you must pass on to the recipients the same
freedoms that you received.  You must make sure that they, too, receive
or can get the source code.  And you must show them these terms so they
know their rights.

  Developers that use the GNU GPL protect your rights with two steps:
(1) assert copyright on the software, and (2) offer you this License
giving you legal permission to copy, distribute and/or modify it.

  For the developers' and authors' protection, the GPL clearly explains
that there is no warranty for this free software.  For both users' and
authors' sake, the GPL requires that modified versions be marked as
changed, so that their problems will not be attributed erroneously to
authors of previous versions.

  Some devices are designed to deny users access to install or run
modified versions of the software inside them, although the manufacturer
can do so.  This is fundamentally incompatible with the aim of
protecting users' freedom to change the software.  The systematic
pattern of such abuse occurs in the area of products for individuals to
use, which is precisely where it is most unacceptable.  Therefore, we
have designed this version of the GPL to prohibit the practice for those
products.  If such problems arise substantially in other domains, we
stand ready to extend this provision to those domains in future versions
of the GPL, as needed to protect the freedom of users.

  Finally, every program is threatened constantly by software patents.
States should not allow patents to restrict development and use of
software on general-purpose computers, but in those that do, we wish to
avoid the special danger that patents applied to a free program could
make it effectively proprietary.  To prevent this, the GPL assures that
patents cannot be used to render the program non-free.

  The precise terms and conditions for copying, distribution and
modification follow.

                       TERMS AND CONDITIONS

  0. Definitions.

  "This License" refers to version 3 of the GNU General Public License.

  "Copyright" also means copyright-like laws that apply to other kinds of
works, such as semiconductor masks.

  "The Program" refers to any copyrightable work licensed under this
License.  Each licensee is addressed as "you".  "Licensees" and
"recipients" may be individuals or organizations.

  To "modify" a work means to copy from or adapt all or part of the work
in a fashion requiring copyright permission, other than the making of an
exact copy.  The resulting work is called a "modified version" of the
earlier work or a work "based on" the earlier work.

  A "covered work" means either the unmodified Program or a work based
on the Program.

  To "propagate" a work means to do anything with it that, without
permission, would make you directly or secondarily liable for
infringement under applicable copyright law, except executing it on a
computer or modifying a private copy.  Propagation includes copying,
distribution (with or without modification), making available to the
public, and in some countries other activities as well.

  To "convey" a work means any kind of propagation that enables other
parties to make or receive copies.  Mere interaction with a user through
a computer network, with no transfer of a copy, is not conveying.

  An interactive user interface displays "Appropriate Legal Notices"
to the extent that it includes a convenient and prominently visible
feature that (1) displays an appropriate copyright notice, and (2)
tells the user that there is no warranty for the work (except to the
extent that warranties are provided), that licensees may convey the
work under this License, and how to view a copy of this License.  If
the interface presents a list of user commands or options, such as a
menu, a prominent item in the list meets this criterion.

  1. Source Code.

  The "source code" for a work means the preferred form of the work
for making modifications to it.  "Object code" means any non-source
form of a work.

  A "Standard Interface" means an interface that either is an official
standard defined by a recognized standards body, or, in the case of
interfaces specified for a particular programming language, one that
is widely used among developers working in that language.

  The "System Libraries" of an executable work include anything, other
than the work as a whole, that (a) is included in the normal form of
packaging a Major Component, but which is not part of that Major
Component, and (b) serves only to enable use of the work with that
Major Component, or to implement a Standard Interface for which an
implementation is available to the public in source code form.  A
"Major Component", in this context, means a major essential component
(kernel, window system, and so on) of the specific operating system
(if any) on which the executable work runs, or a compiler used to
produce the work, or an object code interpreter used to run it.

  The "Corresponding Source" for a work in object code form means all
the source code needed to generate, install, and (for an executable
work) run the object code and to modify the work, including scripts to
control those activities.  However, it does not include the work's
System Libraries, or general-purpose tools or generally available free
programs which are used unmodified in performing those activities but
which are not part of the work.  For example, Corresponding Source
includes interface definition files associated with source files for
the work, and the source code for shared libraries and dynamically
linked subprograms that the work is specifically designed to require,
such as by intimate data communication or control flow between those
subprograms and other parts of the work.

  The Corresponding Source need not include anything that users
can regenerate automatically from other parts of the Corresponding
Source.

  The Corresponding Source for a work in source code form is that
same work.

  2. Basic Permissions.

  All rights granted under this License are granted for the term of
copyright on the Program, and are irrevocable provided the stated
conditions are met.  This License explicitly affirms your unlimited
permission to run the unmodified Program.  The output from running a
covered work is covered by this License only if the output, given its
content, constitutes a covered work.  This License acknowledges your
rights of fair use or other equivalent, as provided by copyright law.

  You may make, run and propagate covered works that you do not
convey, without conditions so long as your license otherwise remains
in force.  You may convey covered works to others for the sole purpose
of having them make modifications exclusively for you, or provide you
with facilities for running those works, provided that you comply with
the terms of this License in conveying all material for which you do
not control copyright.  Those thus making or running the covered works
for you must do so exclusively on your behalf, under your direction
and control, on terms that prohibit them from making any copies of
your copyrighted material outside their relationship with you.

  Conveying under any other circumstances is permitted solely under
the conditions stated below.  Sublicensing is not allowed; section 10
makes it unnecessary.

  3. Protecting Users' Legal Rights From Anti-Circumvention Law.

  No covered work shall be deemed part of an effective technological
measure under any applicable law fulfilling obligations under article
11 of the WIPO copyright treaty adopted on 20 December 1996, or
similar laws prohibiting or restricting circumvention of such
measures.

  When you convey a covered work, you waive any legal power to forbid
circumvention of technological measures to the extent such circumvention
is effected by exercising rights under this License with respect to
the covered work, and you disclaim any intention to limit operation or
modification of the work as a means of enforcing, against the work's
users, your or third parties' legal rights to forbid circumvention of
technological measures.

  4. Conveying Verbatim Copies.

  You may convey verbatim copies of the Program's source code as you
receive it, in any medium, provided that you conspicuously and
appropriately publish on each copy an appropriate copyright notice;
keep intact all notices stating that this License and any
non-permissive terms added in accord with section 7 apply to the code;
keep intact all notices of the absence of any warranty; and give all
recipients a copy of this License along with the Program.

  You may charge any price or no price for each copy that you convey,
and you may offer support or warranty protection for a fee.

  5. Conveying Modified Source Versions.

  You may convey a work based on the Program, or the modifications to
produce it from the Program, in the form of source code under the
terms of section 4, provided that you also meet all of these conditions:

    a) The work must carry prominent notices stating that you modified
    it, and giving a relevant date.

    b) The work must carry prominent notices stating that it is
    released under this License and any conditions added under section
    7.  This requirement modifies the requirement in section 4 to
    "keep intact all notices".

    c) You must license the entire work, as a whole, under this
    License to anyone who comes into possession of a copy.  This
    License will therefore apply, along with any applicable section 7
    additional terms, to the whole of the work, and all its parts,
    regardless of how they are packaged.  This License gives no
    permission to license the work in any other way, but it does not
    invalidate such permission if you have separately received it.

    d) If the work has interactive user interfaces, each must display
    Appropriate Legal Notices; however, if the Program has interactive
    interfaces that do not display Appropriate Legal Notices, your
    work need not make them do so.

  A compilation of a covered work with other separate and independent
works, which are not by their nature extensions of the covered work,
and which are not combined with it such as to form a larger program,
in or on a volume of a storage or distribution medium, is called an
"aggregate" if the compilation and its resulting copyright are not
used to limit the access or legal rights of the compilation's users
beyond what the individual works permit.  Inclusion of a covered work
in an aggregate does not cause this License to apply to the other
parts of the aggregate.

  6. Conveying Non-Source Forms.

  You may convey a covered work in object code form under the terms
of sections 4 and 5, provided that you also convey the
machine-readable Corresponding Source under the terms of this License,
in one of these ways:

    a) Convey the object code in, or embodied in, a physical product
    (including a physical distribution medium), accompanied by the
    Corresponding Source fixed on a durable physical medium
    customarily used for software interchange.

    b) Convey the object code in, or embodied in, a physical product
    (including a physical distribution medium), accompanied by a
    written offer, valid for at least three years and valid for as
    long as you offer spare parts or customer support for that product
    model, to give anyone who possesses the object code either (1) a
    copy of the Corresponding Source for all the software in the
    product that is covered by this License, on a durable physical
    medium customarily used for software interchange, for a price no
    more than your reasonable cost of physically performing this
    conveying of source, or (2) access to copy the
    Corresponding Source from a network server at no charge.

    c) Convey individual copies of the object code with a copy of the
    written offer to provide the Corresponding Source.  This
    alternative is allowed only occasionally and noncommercially, and
    only if you received the object code with such an offer, in accord
    with subsection 6b.

    d) Convey the object code by offering access from a designated
    place (gratis or for a charge), and offer equivalent access to the
    Corresponding Source in the same way through the same place at no
    further charge.  You need not require recipients to copy the
    Corresponding Source along with the object code.  If the place to
    copy the object code is a network server, the Corresponding Source
    may be on a different server (operated by you or a third party)
    that supports equivalent copying facilities, provided you maintain
    clear directions next to the object code saying where to find the
    Corresponding Source.  Regardless of what server hosts the
    Corresponding Source, you remain obligated to ensure that it is
    available for as long as needed to satisfy these requirements.

    e) Convey the object code using peer-to-peer transmission, provided
    you inform other peers where the object code and Corresponding
    Source of the work are being offered to the general public at no
    charge under subsection 6d.

  A separable portion of the object code, whose source code is excluded
from the Corresponding Source as a System Library, need not be
included in conveying the object code work.

  A "User Product" is either (1) a "consumer product", which means any
tangible personal property which is normally used for personal, family,
or household purposes, or (2) anything designed or sold for incorporation
into a dwelling.  In determining whether a product is a consumer product,
doubtful cases shall be resolved in favor of coverage.  For a particular
product received by a particular user, "normally used" refers to a
typical or common use of that class of product, regardless of the status
of the particular user or of the way in which the particular user
actually uses, or expects or is expected to use, the product.  A product
is a consumer product regardless of whether the product has substantial
commercial, industrial or non-consumer uses, unless such uses represent
the only significant mode of use of the product.

  "Installation Information" for a User Product means any methods,
procedures, authorization keys, or other information required to install
and execute modified versions of a covered work in that User Product from
a modified version of its Corresponding Source.  The information must
suffice to ensure that the continued functioning of the modified object
code is in no case prevented or interfered with solely because
modification has been made.

  If you convey an object code work under this section in, or with, or
specifically for use in, a User Product, and the conveying occurs as
part of a transaction in which the right of possession and use of the
User Product is transferred to the recipient in perpetuity or for a
fixed term (regardless of how the transaction is characterized), the
Corresponding Source conveyed under this section must be accompanied
by the Installation Information.  But this requirement does not apply
if neither you nor any third party retains the ability to install
modified object code on the User Product (for example, the work has
been installed in ROM).

  The requirement to provide Installation Information does not include a
requirement to continue to provide support service, warranty, or updates
for a work that has been modified or installed by the recipient, or for
the User Product in which it has been modified or installed.  Access to a
network may be denied when the modification itself materially and
adversely affects the operation of the network or violates the rules and
protocols for communication across the network.

  Corresponding Source conveyed, and Installation Information provided,
in accord with this section must be in a format that is publicly
documented (and with an implementation available to the public in
source code form), and must require no special password or key for
unpacking, reading or copying.

  7. Additional Terms.

  "Additional permissions" are terms that supplement the terms of this
License by making exceptions from one or more of its conditions.
Additional permissions that are applicable to the entire Program shall
be treated as though they were included in this License, to the extent
that they are valid under applicable law.  If additional permissions
apply only to part of the Program, that part may be used separately
under those permissions, but the entire Program remains governed by
this License without regard to the additional permissions.

  When you convey a copy of a covered work, you may at your option
remove any additional permissions from that copy, or from any part of
it.  (Additional permissions may be written to require their own
removal in certain cases when you modify the work.)  You may place
additional permissions on material, added by you to a covered work,
for which you have or can give appropriate copyright permission.

  Notwithstanding any other provision of this License, for material you
add to a covered work, you may (if authorized by the copyright holders of
that material) supplement the terms of this License with terms:

    a) Disclaiming warranty or limiting liability differently from the
    terms of sections 15 and 16 of this License; or

    b) Requiring preservation of specified reasonable legal notices or
    author attributions in that material or in the Appropriate Legal
    Notices displayed by works containing it; or

    c) Prohibiting misrepresentation of the origin of that material, or
    requiring that modified versions of such material be marked in
    reasonable ways as different from the original version; or

    d) Limiting the use for publicity purposes of names of licensors or
    authors of the material; or

    e) Declining to grant rights under trademark law for use of some
    trade names, trademarks, or service marks; or

    f) Requiring indemnification of licensors and authors of that
    material by anyone who conveys the material (or modified versions of
    it) with contractual assumptions of liability to the recipient, for
    any liability that these contractual assumptions directly impose on
    those licensors and authors.

  All other non-permissive additional terms are considered "further
restrictions" within the meaning of section 10.  If the Program as you
received it, or any part of it, contains a notice stating that it is
governed by this License along with a term that is a further
restriction, you may remove that term.  If a license document contains
a further restriction but permits relicensing or conveying under this
License, you may add to a covered work material governed by the terms
of that license document, provided that the further restriction does
not survive such relicensing or conveying.

  If you add terms to a covered work in accord with this section, you
must place, in the relevant source files, a statement of the
additional terms that apply to those files, or a notice indicating
where to find the applicable terms.

  Additional terms, permissive or non-permissive, may be stated in the
form of a separately written license, or stated as exceptions;
the above requirements apply either way.

  8. Termination.

  You may not propagate or modify a covered work except as expressly
provided under this License.  Any attempt otherwise to propagate or
modify it is void, and will automatically terminate your rights under
this License (including any patent licenses granted under the third
paragraph of section 11).

  However, if you cease all violation of this License, then your
license from a particular copyright holder is reinstated (a)
provisionally, unless and until the copyright holder explicitly and
finally terminates your license, and (b) permanently, if the copyright
holder fails to notify you of the violation by some reasonable means
prior to 60 days after the cessation.

  Moreover, your license from a particular copyright holder is
reinstated permanently if the copyright holder notifies you of the
violation by some reasonable means, this is the first time you have
received notice of violation of this License (for any work) from that
copyright holder, and you cure the violation prior to 30 days after
your receipt of the notice.

  Termination of your rights under this section does not terminate the
licenses of parties who have received copies or rights from you under
this License.  If your rights have been terminated and not permanently
reinstated, you do not qualify to receive new licenses for the same
material under section 10.

  9. Acceptance Not Required for Having Copies.

  You are not required to accept this License in order to receive or
run a copy of the Program.  Ancillary propagation of a covered work
occurring solely as a consequence of using peer-to-peer transmission
to receive a copy likewise does not require acceptance.  However,
nothing other than this License grants you permission to propagate or
modify any covered work.  These actions infringe copyright if you do
not accept this License.  Therefore, by modifying or propagating a
covered work, you indicate your acceptance of this License to do so.

  10. Automatic Licensing of Downstream Recipients.

  Each time you convey a covered work, the recipient automatically
receives a license from the original licensors, to run, modify and
propagate that work, subject to this License.  You are not responsible
for enforcing compliance by third parties with this License.

  An "entity transaction" is a transaction transferring control of an
organization, or substantially all assets of one, or subdividing an
organization, or merging organizations.  If propagation of a covered
work results from an entity transaction, each party to that
transaction who receives a copy of the work also receives whatever
licenses to the work the party's predecessor in interest had or could
give under the previous paragraph, plus a right to possession of the
Corresponding Source of the work from the predecessor in interest, if
the predecessor has it or can get it with reasonable efforts.

  You may not impose any further restrictions on the exercise of the
rights granted or affirmed under this License.  For example, you may
not impose a license fee, royalty, or other charge for exercise of
rights granted under this License, and you may not initiate litigation
(including a cross-claim or counterclaim in a lawsuit) alleging that
any patent claim is infringed by making, using, selling, offering for
sale, or importing the Program or any portion of it.

  11. Patents.

  A "contributor" is a copyright holder who authorizes use under this
License of the Program or a work on which the Program is based.  The
work thus licensed is called the contributor's "contributor version".

  A contributor's "essential patent claims" are all patent claims
owned or controlled by the contributor, whether already acquired or
hereafter acquired, that would be infringed by some manner, permitted
by this License, of making, using, or selling its contributor version,
but do not include claims that would be infringed only as a
consequence of further modification of the contributor version.  For
purposes of this definition, "control" includes the right to grant
patent sublicenses in a manner consistent with the requirements of
this License.

  Each contributor grants you a non-exclusive, worldwide, royalty-free
patent license under the contributor's essential patent claims, to
make, use, sell, offer for sale, import and otherwise run, modify and
propagate the contents of its contributor version.

  In the following three paragraphs, a "patent license" is any express
agreement or commitment, however denominated, not to enforce a patent
(such as an express permission to practice a patent or covenant not to
sue for patent infringement).  To "grant" such a patent license to a
party means to make such an agreement or commitment not to enforce a
patent against the party.

  If you convey a covered work, knowingly relying on a patent license,
and the Corresponding Source of the work is not available for anyone
to copy, free of charge and under the terms of this License, through a
publicly available network server or other readily accessible means,
then you must either (1) cause the Corresponding Source to be so
available, or (2) arrange to deprive yourself of the benefit of the
patent license for this particular work, or (3) arrange, in a manner
consistent with the requirements of this License, to extend the patent
license to downstream recipients.  "Knowingly relying" means you have
actual knowledge that, but for the patent license, your conveying the
covered work in a country, or your recipient's use of the covered work
in a country, would infringe one or more identifiable patents in that
country that you have reason to believe are valid.

  If, pursuant to or in connection with a single transaction or
arrangement, you convey, or propagate by procuring conveyance of, a
covered work, and grant a patent license to some of the parties
receiving the covered work authorizing them to use, propagate, modify
or convey a specific copy of the covered work, then the patent license
you grant is automatically extended to all recipients of the covered
work and works based on it.

  A patent license is "discriminatory" if it does not include within
the scope of its coverage, prohibits the exercise of, or is
conditioned on the non-exercise of one or more of the rights that are
specifically granted under this License.  You may not convey a covered
work if you are a party to an arrangement with a third party that is
in the business of distributing software, under which you make payment
to the third party based on the extent of your activity of conveying
the work, and under which the third party grants, to any of the
parties who would receive the covered work from you, a discriminatory
patent license (a) in connection with copies of the covered work
conveyed by you (or copies made from those copies), or (b) primarily
for and in connection with specific products or compilations that
contain the covered work, unless you entered into that arrangement,
or that patent license was granted, prior to 28 March 2007.

  Nothing in this License shall be construed as excluding or limiting
any implied license or other defenses to infringement that may
otherwise be available to you under applicable patent law.

  12. No Surrender of Others' Freedom.

  If conditions are imposed on you (whether by court order, agreement or
otherwise) that contradict the conditions of this License, they do not
excuse you from the conditions of this License.  If you cannot convey a
covered work so as to satisfy simultaneously your obligations under this
License and any other pertinent obligations, then as a consequence you may
not convey it at all.  For example, if you agree to terms that obligate you
to collect a royalty for further conveying from those to whom you convey
the Program, the only way you could satisfy both those terms and this
License would be to refrain entirely from conveying the Program.

  13. Use with the GNU Affero General Public License.

  Notwithstanding any other provision of this License, you have
permission to link or combine any covered work with a work licensed
under version 3 of the GNU Affero General Public License into a single
combined work, and to convey the resulting work.  The terms of this
License will continue to apply to the part which is the covered work,
but the special requirements of the GNU Affero General Public License,
section 13, concerning interaction through a network will apply to the
combination as such.

  14. Revised Versions of this License.

  The Free Software Foundation may publish revised and/or new versions of
the GNU General Public License from time to time.  Such new versions will
be similar in spirit to the present version, but may differ in detail to
address new problems or concerns.

  Each version is given a distinguishing version number.  If the
Program specifies that a certain numbered version of the GNU General
Public License "or any later version" applies to it, you have the
option of following the terms and conditions either of that numbered
version or of any later version published by the Free Software
Foundation.  If the Program does not specify a version number of the
GNU General Public License, you may choose any version ever published
by the Free Software Foundation.

  If the Program specifies that a proxy can decide which future
versions of the GNU General Public License can be used, that proxy's
public statement of acceptance of a version permanently authorizes you
to choose that version for the Program.

  Later license versions may give you additional or different
permissions.  However, no additional obligations are imposed on any
author or copyright holder as a result of your choosing to follow a
later version.

  15. Disclaimer of Warranty.

  THERE IS NO WARRANTY FOR THE PROGRAM, TO THE EXTENT PERMITTED BY
APPLICABLE LAW.  EXCEPT WHEN OTHERWISE STATED IN WRITING THE COPYRIGHT
HOLDERS AND/OR OTHER PARTIES PROVIDE THE PROGRAM "AS IS" WITHOUT WARRANTY
OF ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING, BUT NOT LIMITED TO,
THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
PURPOSE.  THE ENTIRE RISK AS TO THE QUALITY AND PERFORMANCE OF THE PROGRAM
IS WITH YOU.  SHOULD THE PROGRAM PROVE DEFECTIVE, YOU ASSUME THE COST OF
ALL NECESSARY SERVICING, REPAIR OR CORRECTION.

  16. Limitation of Liability.

  IN NO EVENT UNLESS REQUIRED BY APPLICABLE LAW OR AGREED TO IN WRITING
WILL ANY COPYRIGHT HOLDER, OR ANY OTHER PARTY WHO MODIFIES AND/OR CONVEYS
THE PROGRAM AS PERMITTED ABOVE, BE LIABLE TO YOU FOR DAMAGES, INCLUDING ANY
GENERAL, SPECIAL, INCIDENTAL OR CONSEQUENTIAL DAMAGES ARISING OUT OF THE
USE OR INABILITY TO USE THE PROGRAM (INCLUDING BUT NOT LIMITED TO LOSS OF
DATA OR DATA BEING RENDERED INACCURATE OR LOSSES SUSTAINED BY YOU OR THIRD
PARTIES OR A FAILURE OF THE PROGRAM TO OPERATE WITH ANY OTHER PROGRAMS),
EVEN IF SUCH HOLDER OR OTHER PARTY HAS BEEN ADVISED OF THE POSSIBILITY OF
SUCH DAMAGES.

  17. Interpretation of Sections 15 and 16.

  If the disclaimer of warranty and limitation of liability provided
above cannot be given local legal effect according to their terms,
reviewing courts shall apply local law that most closely approximates
an absolute waiver of all civil liability in connection with the
Program, unless a warranty or assumption of liability accompanies a
copy of the Program in return for a fee.

                     END OF TERMS AND CONDITIONS

            How to Apply These Terms to Your New Programs

  If you develop a new program, and you want it to be of the greatest
possible use to the public, the best way to achieve this is to make it
free software which everyone can redistribute and change under these terms.

  To do so, attach the following notices to the program.  It is safest
to attach them to the start of each source file to most effectively
state the exclusion of warranty; and each file should have at least
the "copyright" line and a pointer to where the full notice is found.

    {one line to give the program's name and a brief idea of what it does.}
    Copyright (C) {year}  {name of author}

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.

Also add information on how to contact you by electronic and paper mail.

  If the program does terminal interaction, make it output a short
notice like this when it starts in an interactive mode:

    {project}  Copyright (C) {year}  {fullname}
    This program comes with ABSOLUTELY NO WARRANTY; for details type `show w'.
    This is free software, and you are welcome to redistribute it
    under certain conditions; type `show c' for details.

The hypothetical commands `show w' and `show c' should show the appropriate
parts of the General Public License.  Of course, your program's commands
might be different; for a GUI interface, you would use an "about box".

  You should also get your employer (if you work as a programmer) or school,
if any, to sign a "copyright disclaimer" for the program, if necessary.
For more information on this, and how to apply and follow the GNU GPL, see
<http://www.gnu.org/licenses/>.

  The GNU General Public License does not permit incorporating your program
into proprietary programs.  If your program is a subroutine library, you
may consider it more useful to permit linking proprietary applications with
the library.  If this is what you want to do, use the GNU Lesser General
Public License instead of this License.  But first, please read
<http://www.gnu.org/philosophy/why-not-lgpl.html>.
~~~~~~~~~~

### Licence text B - GNU Lesser General Public License, version 2.1

Text copied from: ESP8266Audio 2.2.0, file src/libflac/COPYING.LGPL

~~~~~~~~~~
		  GNU LESSER GENERAL PUBLIC LICENSE
		       Version 2.1, February 1999

 Copyright (C) 1991, 1999 Free Software Foundation, Inc.
 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 Everyone is permitted to copy and distribute verbatim copies
 of this license document, but changing it is not allowed.

[This is the first released version of the Lesser GPL.  It also counts
 as the successor of the GNU Library Public License, version 2, hence
 the version number 2.1.]

			    Preamble

  The licenses for most software are designed to take away your
freedom to share and change it.  By contrast, the GNU General Public
Licenses are intended to guarantee your freedom to share and change
free software--to make sure the software is free for all its users.

  This license, the Lesser General Public License, applies to some
specially designated software packages--typically libraries--of the
Free Software Foundation and other authors who decide to use it.  You
can use it too, but we suggest you first think carefully about whether
this license or the ordinary General Public License is the better
strategy to use in any particular case, based on the explanations below.

  When we speak of free software, we are referring to freedom of use,
not price.  Our General Public Licenses are designed to make sure that
you have the freedom to distribute copies of free software (and charge
for this service if you wish); that you receive source code or can get
it if you want it; that you can change the software and use pieces of
it in new free programs; and that you are informed that you can do
these things.

  To protect your rights, we need to make restrictions that forbid
distributors to deny you these rights or to ask you to surrender these
rights.  These restrictions translate to certain responsibilities for
you if you distribute copies of the library or if you modify it.

  For example, if you distribute copies of the library, whether gratis
or for a fee, you must give the recipients all the rights that we gave
you.  You must make sure that they, too, receive or can get the source
code.  If you link other code with the library, you must provide
complete object files to the recipients, so that they can relink them
with the library after making changes to the library and recompiling
it.  And you must show them these terms so they know their rights.

  We protect your rights with a two-step method: (1) we copyright the
library, and (2) we offer you this license, which gives you legal
permission to copy, distribute and/or modify the library.

  To protect each distributor, we want to make it very clear that
there is no warranty for the free library.  Also, if the library is
modified by someone else and passed on, the recipients should know
that what they have is not the original version, so that the original
author's reputation will not be affected by problems that might be
introduced by others.

  Finally, software patents pose a constant threat to the existence of
any free program.  We wish to make sure that a company cannot
effectively restrict the users of a free program by obtaining a
restrictive license from a patent holder.  Therefore, we insist that
any patent license obtained for a version of the library must be
consistent with the full freedom of use specified in this license.

  Most GNU software, including some libraries, is covered by the
ordinary GNU General Public License.  This license, the GNU Lesser
General Public License, applies to certain designated libraries, and
is quite different from the ordinary General Public License.  We use
this license for certain libraries in order to permit linking those
libraries into non-free programs.

  When a program is linked with a library, whether statically or using
a shared library, the combination of the two is legally speaking a
combined work, a derivative of the original library.  The ordinary
General Public License therefore permits such linking only if the
entire combination fits its criteria of freedom.  The Lesser General
Public License permits more lax criteria for linking other code with
the library.

  We call this license the "Lesser" General Public License because it
does Less to protect the user's freedom than the ordinary General
Public License.  It also provides other free software developers Less
of an advantage over competing non-free programs.  These disadvantages
are the reason we use the ordinary General Public License for many
libraries.  However, the Lesser license provides advantages in certain
special circumstances.

  For example, on rare occasions, there may be a special need to
encourage the widest possible use of a certain library, so that it becomes
a de-facto standard.  To achieve this, non-free programs must be
allowed to use the library.  A more frequent case is that a free
library does the same job as widely used non-free libraries.  In this
case, there is little to gain by limiting the free library to free
software only, so we use the Lesser General Public License.

  In other cases, permission to use a particular library in non-free
programs enables a greater number of people to use a large body of
free software.  For example, permission to use the GNU C Library in
non-free programs enables many more people to use the whole GNU
operating system, as well as its variant, the GNU/Linux operating
system.

  Although the Lesser General Public License is Less protective of the
users' freedom, it does ensure that the user of a program that is
linked with the Library has the freedom and the wherewithal to run
that program using a modified version of the Library.

  The precise terms and conditions for copying, distribution and
modification follow.  Pay close attention to the difference between a
"work based on the library" and a "work that uses the library".  The
former contains code derived from the library, whereas the latter must
be combined with the library in order to run.

		  GNU LESSER GENERAL PUBLIC LICENSE
   TERMS AND CONDITIONS FOR COPYING, DISTRIBUTION AND MODIFICATION

  0. This License Agreement applies to any software library or other
program which contains a notice placed by the copyright holder or
other authorized party saying it may be distributed under the terms of
this Lesser General Public License (also called "this License").
Each licensee is addressed as "you".

  A "library" means a collection of software functions and/or data
prepared so as to be conveniently linked with application programs
(which use some of those functions and data) to form executables.

  The "Library", below, refers to any such software library or work
which has been distributed under these terms.  A "work based on the
Library" means either the Library or any derivative work under
copyright law: that is to say, a work containing the Library or a
portion of it, either verbatim or with modifications and/or translated
straightforwardly into another language.  (Hereinafter, translation is
included without limitation in the term "modification".)

  "Source code" for a work means the preferred form of the work for
making modifications to it.  For a library, complete source code means
all the source code for all modules it contains, plus any associated
interface definition files, plus the scripts used to control compilation
and installation of the library.

  Activities other than copying, distribution and modification are not
covered by this License; they are outside its scope.  The act of
running a program using the Library is not restricted, and output from
such a program is covered only if its contents constitute a work based
on the Library (independent of the use of the Library in a tool for
writing it).  Whether that is true depends on what the Library does
and what the program that uses the Library does.
  
  1. You may copy and distribute verbatim copies of the Library's
complete source code as you receive it, in any medium, provided that
you conspicuously and appropriately publish on each copy an
appropriate copyright notice and disclaimer of warranty; keep intact
all the notices that refer to this License and to the absence of any
warranty; and distribute a copy of this License along with the
Library.

  You may charge a fee for the physical act of transferring a copy,
and you may at your option offer warranty protection in exchange for a
fee.

  2. You may modify your copy or copies of the Library or any portion
of it, thus forming a work based on the Library, and copy and
distribute such modifications or work under the terms of Section 1
above, provided that you also meet all of these conditions:

    a) The modified work must itself be a software library.

    b) You must cause the files modified to carry prominent notices
    stating that you changed the files and the date of any change.

    c) You must cause the whole of the work to be licensed at no
    charge to all third parties under the terms of this License.

    d) If a facility in the modified Library refers to a function or a
    table of data to be supplied by an application program that uses
    the facility, other than as an argument passed when the facility
    is invoked, then you must make a good faith effort to ensure that,
    in the event an application does not supply such function or
    table, the facility still operates, and performs whatever part of
    its purpose remains meaningful.

    (For example, a function in a library to compute square roots has
    a purpose that is entirely well-defined independent of the
    application.  Therefore, Subsection 2d requires that any
    application-supplied function or table used by this function must
    be optional: if the application does not supply it, the square
    root function must still compute square roots.)

These requirements apply to the modified work as a whole.  If
identifiable sections of that work are not derived from the Library,
and can be reasonably considered independent and separate works in
themselves, then this License, and its terms, do not apply to those
sections when you distribute them as separate works.  But when you
distribute the same sections as part of a whole which is a work based
on the Library, the distribution of the whole must be on the terms of
this License, whose permissions for other licensees extend to the
entire whole, and thus to each and every part regardless of who wrote
it.

Thus, it is not the intent of this section to claim rights or contest
your rights to work written entirely by you; rather, the intent is to
exercise the right to control the distribution of derivative or
collective works based on the Library.

In addition, mere aggregation of another work not based on the Library
with the Library (or with a work based on the Library) on a volume of
a storage or distribution medium does not bring the other work under
the scope of this License.

  3. You may opt to apply the terms of the ordinary GNU General Public
License instead of this License to a given copy of the Library.  To do
this, you must alter all the notices that refer to this License, so
that they refer to the ordinary GNU General Public License, version 2,
instead of to this License.  (If a newer version than version 2 of the
ordinary GNU General Public License has appeared, then you can specify
that version instead if you wish.)  Do not make any other change in
these notices.

  Once this change is made in a given copy, it is irreversible for
that copy, so the ordinary GNU General Public License applies to all
subsequent copies and derivative works made from that copy.

  This option is useful when you wish to copy part of the code of
the Library into a program that is not a library.

  4. You may copy and distribute the Library (or a portion or
derivative of it, under Section 2) in object code or executable form
under the terms of Sections 1 and 2 above provided that you accompany
it with the complete corresponding machine-readable source code, which
must be distributed under the terms of Sections 1 and 2 above on a
medium customarily used for software interchange.

  If distribution of object code is made by offering access to copy
from a designated place, then offering equivalent access to copy the
source code from the same place satisfies the requirement to
distribute the source code, even though third parties are not
compelled to copy the source along with the object code.

  5. A program that contains no derivative of any portion of the
Library, but is designed to work with the Library by being compiled or
linked with it, is called a "work that uses the Library".  Such a
work, in isolation, is not a derivative work of the Library, and
therefore falls outside the scope of this License.

  However, linking a "work that uses the Library" with the Library
creates an executable that is a derivative of the Library (because it
contains portions of the Library), rather than a "work that uses the
library".  The executable is therefore covered by this License.
Section 6 states terms for distribution of such executables.

  When a "work that uses the Library" uses material from a header file
that is part of the Library, the object code for the work may be a
derivative work of the Library even though the source code is not.
Whether this is true is especially significant if the work can be
linked without the Library, or if the work is itself a library.  The
threshold for this to be true is not precisely defined by law.

  If such an object file uses only numerical parameters, data
structure layouts and accessors, and small macros and small inline
functions (ten lines or less in length), then the use of the object
file is unrestricted, regardless of whether it is legally a derivative
work.  (Executables containing this object code plus portions of the
Library will still fall under Section 6.)

  Otherwise, if the work is a derivative of the Library, you may
distribute the object code for the work under the terms of Section 6.
Any executables containing that work also fall under Section 6,
whether or not they are linked directly with the Library itself.

  6. As an exception to the Sections above, you may also combine or
link a "work that uses the Library" with the Library to produce a
work containing portions of the Library, and distribute that work
under terms of your choice, provided that the terms permit
modification of the work for the customer's own use and reverse
engineering for debugging such modifications.

  You must give prominent notice with each copy of the work that the
Library is used in it and that the Library and its use are covered by
this License.  You must supply a copy of this License.  If the work
during execution displays copyright notices, you must include the
copyright notice for the Library among them, as well as a reference
directing the user to the copy of this License.  Also, you must do one
of these things:

    a) Accompany the work with the complete corresponding
    machine-readable source code for the Library including whatever
    changes were used in the work (which must be distributed under
    Sections 1 and 2 above); and, if the work is an executable linked
    with the Library, with the complete machine-readable "work that
    uses the Library", as object code and/or source code, so that the
    user can modify the Library and then relink to produce a modified
    executable containing the modified Library.  (It is understood
    that the user who changes the contents of definitions files in the
    Library will not necessarily be able to recompile the application
    to use the modified definitions.)

    b) Use a suitable shared library mechanism for linking with the
    Library.  A suitable mechanism is one that (1) uses at run time a
    copy of the library already present on the user's computer system,
    rather than copying library functions into the executable, and (2)
    will operate properly with a modified version of the library, if
    the user installs one, as long as the modified version is
    interface-compatible with the version that the work was made with.

    c) Accompany the work with a written offer, valid for at
    least three years, to give the same user the materials
    specified in Subsection 6a, above, for a charge no more
    than the cost of performing this distribution.

    d) If distribution of the work is made by offering access to copy
    from a designated place, offer equivalent access to copy the above
    specified materials from the same place.

    e) Verify that the user has already received a copy of these
    materials or that you have already sent this user a copy.

  For an executable, the required form of the "work that uses the
Library" must include any data and utility programs needed for
reproducing the executable from it.  However, as a special exception,
the materials to be distributed need not include anything that is
normally distributed (in either source or binary form) with the major
components (compiler, kernel, and so on) of the operating system on
which the executable runs, unless that component itself accompanies
the executable.

  It may happen that this requirement contradicts the license
restrictions of other proprietary libraries that do not normally
accompany the operating system.  Such a contradiction means you cannot
use both them and the Library together in an executable that you
distribute.

  7. You may place library facilities that are a work based on the
Library side-by-side in a single library together with other library
facilities not covered by this License, and distribute such a combined
library, provided that the separate distribution of the work based on
the Library and of the other library facilities is otherwise
permitted, and provided that you do these two things:

    a) Accompany the combined library with a copy of the same work
    based on the Library, uncombined with any other library
    facilities.  This must be distributed under the terms of the
    Sections above.

    b) Give prominent notice with the combined library of the fact
    that part of it is a work based on the Library, and explaining
    where to find the accompanying uncombined form of the same work.

  8. You may not copy, modify, sublicense, link with, or distribute
the Library except as expressly provided under this License.  Any
attempt otherwise to copy, modify, sublicense, link with, or
distribute the Library is void, and will automatically terminate your
rights under this License.  However, parties who have received copies,
or rights, from you under this License will not have their licenses
terminated so long as such parties remain in full compliance.

  9. You are not required to accept this License, since you have not
signed it.  However, nothing else grants you permission to modify or
distribute the Library or its derivative works.  These actions are
prohibited by law if you do not accept this License.  Therefore, by
modifying or distributing the Library (or any work based on the
Library), you indicate your acceptance of this License to do so, and
all its terms and conditions for copying, distributing or modifying
the Library or works based on it.

  10. Each time you redistribute the Library (or any work based on the
Library), the recipient automatically receives a license from the
original licensor to copy, distribute, link with or modify the Library
subject to these terms and conditions.  You may not impose any further
restrictions on the recipients' exercise of the rights granted herein.
You are not responsible for enforcing compliance by third parties with
this License.

  11. If, as a consequence of a court judgment or allegation of patent
infringement or for any other reason (not limited to patent issues),
conditions are imposed on you (whether by court order, agreement or
otherwise) that contradict the conditions of this License, they do not
excuse you from the conditions of this License.  If you cannot
distribute so as to satisfy simultaneously your obligations under this
License and any other pertinent obligations, then as a consequence you
may not distribute the Library at all.  For example, if a patent
license would not permit royalty-free redistribution of the Library by
all those who receive copies directly or indirectly through you, then
the only way you could satisfy both it and this License would be to
refrain entirely from distribution of the Library.

If any portion of this section is held invalid or unenforceable under any
particular circumstance, the balance of the section is intended to apply,
and the section as a whole is intended to apply in other circumstances.

It is not the purpose of this section to induce you to infringe any
patents or other property right claims or to contest validity of any
such claims; this section has the sole purpose of protecting the
integrity of the free software distribution system which is
implemented by public license practices.  Many people have made
generous contributions to the wide range of software distributed
through that system in reliance on consistent application of that
system; it is up to the author/donor to decide if he or she is willing
to distribute software through any other system and a licensee cannot
impose that choice.

This section is intended to make thoroughly clear what is believed to
be a consequence of the rest of this License.

  12. If the distribution and/or use of the Library is restricted in
certain countries either by patents or by copyrighted interfaces, the
original copyright holder who places the Library under this License may add
an explicit geographical distribution limitation excluding those countries,
so that distribution is permitted only in or among countries not thus
excluded.  In such case, this License incorporates the limitation as if
written in the body of this License.

  13. The Free Software Foundation may publish revised and/or new
versions of the Lesser General Public License from time to time.
Such new versions will be similar in spirit to the present version,
but may differ in detail to address new problems or concerns.

Each version is given a distinguishing version number.  If the Library
specifies a version number of this License which applies to it and
"any later version", you have the option of following the terms and
conditions either of that version or of any later version published by
the Free Software Foundation.  If the Library does not specify a
license version number, you may choose any version ever published by
the Free Software Foundation.

  14. If you wish to incorporate parts of the Library into other free
programs whose distribution conditions are incompatible with these,
write to the author to ask for permission.  For software which is
copyrighted by the Free Software Foundation, write to the Free
Software Foundation; we sometimes make exceptions for this.  Our
decision will be guided by the two goals of preserving the free status
of all derivatives of our free software and of promoting the sharing
and reuse of software generally.

			    NO WARRANTY

  15. BECAUSE THE LIBRARY IS LICENSED FREE OF CHARGE, THERE IS NO
WARRANTY FOR THE LIBRARY, TO THE EXTENT PERMITTED BY APPLICABLE LAW.
EXCEPT WHEN OTHERWISE STATED IN WRITING THE COPYRIGHT HOLDERS AND/OR
OTHER PARTIES PROVIDE THE LIBRARY "AS IS" WITHOUT WARRANTY OF ANY
KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
PURPOSE.  THE ENTIRE RISK AS TO THE QUALITY AND PERFORMANCE OF THE
LIBRARY IS WITH YOU.  SHOULD THE LIBRARY PROVE DEFECTIVE, YOU ASSUME
THE COST OF ALL NECESSARY SERVICING, REPAIR OR CORRECTION.

  16. IN NO EVENT UNLESS REQUIRED BY APPLICABLE LAW OR AGREED TO IN
WRITING WILL ANY COPYRIGHT HOLDER, OR ANY OTHER PARTY WHO MAY MODIFY
AND/OR REDISTRIBUTE THE LIBRARY AS PERMITTED ABOVE, BE LIABLE TO YOU
FOR DAMAGES, INCLUDING ANY GENERAL, SPECIAL, INCIDENTAL OR
CONSEQUENTIAL DAMAGES ARISING OUT OF THE USE OR INABILITY TO USE THE
LIBRARY (INCLUDING BUT NOT LIMITED TO LOSS OF DATA OR DATA BEING
RENDERED INACCURATE OR LOSSES SUSTAINED BY YOU OR THIRD PARTIES OR A
FAILURE OF THE LIBRARY TO OPERATE WITH ANY OTHER SOFTWARE), EVEN IF
SUCH HOLDER OR OTHER PARTY HAS BEEN ADVISED OF THE POSSIBILITY OF SUCH
DAMAGES.

		     END OF TERMS AND CONDITIONS

           How to Apply These Terms to Your New Libraries

  If you develop a new library, and you want it to be of the greatest
possible use to the public, we recommend making it free software that
everyone can redistribute and change.  You can do so by permitting
redistribution under these terms (or, alternatively, under the terms of the
ordinary General Public License).

  To apply these terms, attach the following notices to the library.  It is
safest to attach them to the start of each source file to most effectively
convey the exclusion of warranty; and each file should have at least the
"copyright" line and a pointer to where the full notice is found.

    <one line to give the library's name and a brief idea of what it does.>
    Copyright (C) <year>  <name of author>

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Lesser General Public
    License as published by the Free Software Foundation; either
    version 2.1 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public
    License along with this library; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA

Also add information on how to contact you by electronic and paper mail.

You should also get your employer (if you work as a programmer) or your
school, if any, to sign a "copyright disclaimer" for the library, if
necessary.  Here is a sample; alter the names:

  Yoyodyne, Inc., hereby disclaims all copyright interest in the
  library `Frob' (a library for tweaking knobs) written by James Random Hacker.

  <signature of Ty Coon>, 1 April 1990
  Ty Coon, President of Vice

That's all there is to it!


~~~~~~~~~~

### Licence text C - Apache License, Version 2.0

Text copied from: ESP32-A2DP, file LICENSE

~~~~~~~~~~
Apache License

Version 2.0, January 2004

http://www.apache.org/licenses/

TERMS AND CONDITIONS FOR USE, REPRODUCTION, AND DISTRIBUTION

1. Definitions.

"License" shall mean the terms and conditions for use, reproduction, and distribution as defined by Sections 1 through 9 of this document.

"Licensor" shall mean the copyright owner or entity authorized by the copyright owner that is granting the License.

"Legal Entity" shall mean the union of the acting entity and all other entities that control, are controlled by, or are under common control with that entity. For the purposes of this definition, "control" means (i) the power, direct or indirect, to cause the direction or management of such entity, whether by contract or otherwise, or (ii) ownership of fifty percent (50%) or more of the outstanding shares, or (iii) beneficial ownership of such entity.

"You" (or "Your") shall mean an individual or Legal Entity exercising permissions granted by this License.

"Source" form shall mean the preferred form for making modifications, including but not limited to software source code, documentation source, and configuration files.

"Object" form shall mean any form resulting from mechanical transformation or translation of a Source form, including but not limited to compiled object code, generated documentation, and conversions to other media types.

"Work" shall mean the work of authorship, whether in Source or Object form, made available under the License, as indicated by a copyright notice that is included in or attached to the work (an example is provided in the Appendix below).

"Derivative Works" shall mean any work, whether in Source or Object form, that is based on (or derived from) the Work and for which the editorial revisions, annotations, elaborations, or other modifications represent, as a whole, an original work of authorship. For the purposes of this License, Derivative Works shall not include works that remain separable from, or merely link (or bind by name) to the interfaces of, the Work and Derivative Works thereof.

"Contribution" shall mean any work of authorship, including the original version of the Work and any modifications or additions to that Work or Derivative Works thereof, that is intentionally submitted to Licensor for inclusion in the Work by the copyright owner or by an individual or Legal Entity authorized to submit on behalf of the copyright owner. For the purposes of this definition, "submitted" means any form of electronic, verbal, or written communication sent to the Licensor or its representatives, including but not limited to communication on electronic mailing lists, source code control systems, and issue tracking systems that are managed by, or on behalf of, the Licensor for the purpose of discussing and improving the Work, but excluding communication that is conspicuously marked or otherwise designated in writing by the copyright owner as "Not a Contribution."

"Contributor" shall mean Licensor and any individual or Legal Entity on behalf of whom a Contribution has been received by Licensor and subsequently incorporated within the Work.

2. Grant of Copyright License. Subject to the terms and conditions of this License, each Contributor hereby grants to You a perpetual, worldwide, non-exclusive, no-charge, royalty-free, irrevocable copyright license to reproduce, prepare Derivative Works of, publicly display, publicly perform, sublicense, and distribute the Work and such Derivative Works in Source or Object form.

3. Grant of Patent License. Subject to the terms and conditions of this License, each Contributor hereby grants to You a perpetual, worldwide, non-exclusive, no-charge, royalty-free, irrevocable (except as stated in this section) patent license to make, have made, use, offer to sell, sell, import, and otherwise transfer the Work, where such license applies only to those patent claims licensable by such Contributor that are necessarily infringed by their Contribution(s) alone or by combination of their Contribution(s) with the Work to which such Contribution(s) was submitted. If You institute patent litigation against any entity (including a cross-claim or counterclaim in a lawsuit) alleging that the Work or a Contribution incorporated within the Work constitutes direct or contributory patent infringement, then any patent licenses granted to You under this License for that Work shall terminate as of the date such litigation is filed.

4. Redistribution. You may reproduce and distribute copies of the Work or Derivative Works thereof in any medium, with or without modifications, and in Source or Object form, provided that You meet the following conditions:

You must give any other recipients of the Work or Derivative Works a copy of this License; and
You must cause any modified files to carry prominent notices stating that You changed the files; and
You must retain, in the Source form of any Derivative Works that You distribute, all copyright, patent, trademark, and attribution notices from the Source form of the Work, excluding those notices that do not pertain to any part of the Derivative Works; and
If the Work includes a "NOTICE" text file as part of its distribution, then any Derivative Works that You distribute must include a readable copy of the attribution notices contained within such NOTICE file, excluding those notices that do not pertain to any part of the Derivative Works, in at least one of the following places: within a NOTICE text file distributed as part of the Derivative Works; within the Source form or documentation, if provided along with the Derivative Works; or, within a display generated by the Derivative Works, if and wherever such third-party notices normally appear. The contents of the NOTICE file are for informational purposes only and do not modify the License. You may add Your own attribution notices within Derivative Works that You distribute, alongside or as an addendum to the NOTICE text from the Work, provided that such additional attribution notices cannot be construed as modifying the License.

You may add Your own copyright statement to Your modifications and may provide additional or different license terms and conditions for use, reproduction, or distribution of Your modifications, or for any such Derivative Works as a whole, provided Your use, reproduction, and distribution of the Work otherwise complies with the conditions stated in this License.
5. Submission of Contributions. Unless You explicitly state otherwise, any Contribution intentionally submitted for inclusion in the Work by You to the Licensor shall be under the terms and conditions of this License, without any additional terms or conditions. Notwithstanding the above, nothing herein shall supersede or modify the terms of any separate license agreement you may have executed with Licensor regarding such Contributions.

6. Trademarks. This License does not grant permission to use the trade names, trademarks, service marks, or product names of the Licensor, except as required for reasonable and customary use in describing the origin of the Work and reproducing the content of the NOTICE file.

7. Disclaimer of Warranty. Unless required by applicable law or agreed to in writing, Licensor provides the Work (and each Contributor provides its Contributions) on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied, including, without limitation, any warranties or conditions of TITLE, NON-INFRINGEMENT, MERCHANTABILITY, or FITNESS FOR A PARTICULAR PURPOSE. You are solely responsible for determining the appropriateness of using or redistributing the Work and assume any risks associated with Your exercise of permissions under this License.

8. Limitation of Liability. In no event and under no legal theory, whether in tort (including negligence), contract, or otherwise, unless required by applicable law (such as deliberate and grossly negligent acts) or agreed to in writing, shall any Contributor be liable to You for damages, including any direct, indirect, special, incidental, or consequential damages of any character arising as a result of this License or out of the use or inability to use the Work (including but not limited to damages for loss of goodwill, work stoppage, computer failure or malfunction, or any and all other commercial damages or losses), even if such Contributor has been advised of the possibility of such damages.

9. Accepting Warranty or Additional Liability. While redistributing the Work or Derivative Works thereof, You may choose to offer, and charge a fee for, acceptance of support, warranty, indemnity, or other liability obligations and/or rights consistent with this License. However, in accepting such obligations, You may act only on Your own behalf and on Your sole responsibility, not on behalf of any other Contributor, and only if You agree to indemnify, defend, and hold each Contributor harmless for any liability incurred by, or claims asserted against, such Contributor by reason of your accepting any such warranty or additional liability.

END OF TERMS AND CONDITIONS

~~~~~~~~~~

### Licence text D - MIT License (Adafruit BusIO)

Text copied from: Adafruit BusIO 1.17.4, file LICENSE

~~~~~~~~~~
The MIT License (MIT)

Copyright (c) 2017 Adafruit Industries

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
~~~~~~~~~~

### Licence text E - MIT License (ArduinoJson)

Text copied from: ArduinoJson 7.4.3, file LICENSE.txt

~~~~~~~~~~
The MIT License (MIT)
---------------------

Copyright © 2014-2026, Benoit BLANCHON

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the “Software”), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED “AS IS”, WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
~~~~~~~~~~

### Licence text F - MIT License (Bounce2)

Text copied from: Bounce2 2.72, file LICENSE

~~~~~~~~~~
The MIT License (MIT)

Copyright (c) 2013 thomasfredericks

Permission is hereby granted, free of charge, to any person obtaining a copy of
this software and associated documentation files (the "Software"), to deal in
the Software without restriction, including without limitation the rights to
use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
the Software, and to permit persons to whom the Software is furnished to do so,
subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
~~~~~~~~~~

### Licence text G - Software License Agreement (BSD License) (Adafruit GFX Library)

Text copied from: Adafruit GFX Library 1.12.6, file license.txt

~~~~~~~~~~
Software License Agreement (BSD License)

Copyright (c) 2012 Adafruit Industries.  All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

- Redistributions of source code must retain the above copyright notice,
  this list of conditions and the following disclaimer.
- Redistributions in binary form must reproduce the above copyright notice,
  this list of conditions and the following disclaimer in the documentation
  and/or other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
POSSIBILITY OF SUCH DAMAGE.
~~~~~~~~~~

### Licence text H - RealNetworks Helix library notice (libhelix-mp3)

Text copied from: ESP8266Audio 2.2.0, file src/libhelix-mp3/LICENSE.txt

~~~~~~~~~~
 Copyright (c) 1995-2004 RealNetworks, Inc. All Rights Reserved.  
        
 The contents of this directory, and (except where otherwise
 indicated) the directories included within this directory, are
 subject to the current version of the RealNetworks Public Source
 License (the "RPSL") available at RPSL.txt in this directory, unless
 you have licensed the directory under the current version of the
 RealNetworks Community Source License (the "RCSL") available at
 RCSL.txt in this directory, in which case the RCSL will apply. You
 may also obtain the license terms directly from RealNetworks.  You
 may not use the files in this directory except in compliance with the
 RPSL or, if you have a valid RCSL with RealNetworks applicable to
 this directory, the RCSL.  Please see the applicable RPSL or RCSL for
 the rights, obligations and limitations governing use of the contents
 of the directory.
 
 This directory is part of the Helix DNA Technology. RealNetworks is
 the developer of the Original Code and owns the copyrights in the
 portions it created.
   
 This directory, and the directories included with this directory, are
 distributed and made available on an 'AS IS' basis, WITHOUT WARRANTY
 OF ANY KIND, EITHER EXPRESS OR IMPLIED, AND REALNETWORKS HEREBY
 DISCLAIMS ALL SUCH WARRANTIES, INCLUDING WITHOUT LIMITATION, ANY
 WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE,
 QUIET ENJOYMENT OR NON-INFRINGEMENT.
  
 Technology Compatibility Kit Test Suite(s) Location:  
    http://www.helixcommunity.org/content/tck  

~~~~~~~~~~

### Licence text I - RealNetworks Public Source License Version 1.0 (RPSL)

Text copied from: ESP8266Audio 2.2.0, file src/libhelix-mp3/RPSL.txt

~~~~~~~~~~
RealNetworks Public Source License Version 1.0
(Rev. Date October 28, 2002)

1. General Definitions. This License applies to any program or other work which
RealNetworks, Inc., or any other entity that elects to use this license,
("Licensor") makes publicly available and which contains a notice placed by
Licensor identifying such program or work as "Original Code" and stating that it
is subject to the terms of this RealNetworks Public Source License version 1.0
(or subsequent version thereof) ("License"). You are not required to accept this
License. However, nothing else grants You permission to use, copy, modify or
distribute the software or its derivative works. These actions are prohibited by
law if You do not accept this License. Therefore, by modifying, copying or
distributing the software (or any work based on the software), You indicate your
acceptance of this License to do so, and all its terms and conditions. In
addition, you agree to the terms of this License by clicking the Accept button
or downloading the software. As used in this License:

1.1 "Applicable Patent Rights" mean: (a) in the case where Licensor is the
grantor of rights, claims of patents that (i) are now or hereafter acquired,
owned by or assigned to Licensor and (ii) are necessarily infringed by using or
making the Original Code alone and not in combination with other software or
hardware; and (b) in the case where You are the grantor of rights, claims of
patents that (i) are now or hereafter acquired, owned by or assigned to You and
(ii) are infringed (directly or indirectly) by using or making Your
Modifications, taken alone or in combination with Original Code.

1.2 "Compatible Source License" means any one of the licenses listed on Exhibit
B or at https://www.helixcommunity.org/content/complicense or other licenses
specifically identified by Licensor in writing. Notwithstanding any term to the
contrary in any Compatible Source License, any code covered by any Compatible
Source License that is used with Covered Code must be made readily available in
Source Code format for royalty-free use under the terms of the Compatible Source
License or this License.

1.3 "Contributor" means any person or entity that creates or contributes to the
creation of Modifications.

1.4 "Covered Code" means the Original Code, Modifications, the combination of
Original Code and any Modifications, and/or any respective portions thereof.

1.5 "Deploy" means to use, sublicense or distribute Covered Code other than for
Your internal research and development (R&D) and/or Personal Use, and includes
without limitation, any and all internal use or distribution of Covered Code
within Your business or organization except for R&D use and/or Personal Use, as
well as direct or indirect sublicensing or distribution of Covered Code by You
to any third party in any form or manner.

1.6 "Derivative Work" means either the Covered Code or any derivative work under
United States copyright law, and including any work containing or including any
portion of the Covered Code or Modifications, either verbatim or with
modifications and/or translated into another language. Derivative Work also
includes any work which combines any portion of Covered Code or Modifications
with code not otherwise governed by the terms of this License.

1.7 "Externally Deploy" means to Deploy the Covered Code in any way that may be
accessed or used by anyone other than You, used to provide any services to
anyone other than You, or used in any way to deliver any content to anyone other
than You, whether the Covered Code is distributed to those parties, made
available as an application intended for use over a computer network, or used to
provide services or otherwise deliver content to anyone other than You.

1.8. "Interface" means interfaces, functions, properties, class definitions,
APIs, header files, GUIDs, V-Tables, and/or protocols allowing one piece of
software, firmware or hardware to communicate or interoperate with another piece
of software, firmware or hardware.

1.9 "Modifications" mean any addition to, deletion from, and/or change to, the
substance and/or structure of the Original Code, any previous Modifications, the
combination of Original Code and any previous Modifications, and/or any
respective portions thereof. When code is released as a series of files, a
Modification is: (a) any addition to or deletion from the contents of a file
containing Covered Code; and/or (b) any new file or other representation of
computer program statements that contains any part of Covered Code.

1.10 "Original Code" means (a) the Source Code of a program or other work as
originally made available by Licensor under this License, including the Source
Code of any updates or upgrades to such programs or works made available by
Licensor under this License, and that has been expressly identified by Licensor
as such in the header file(s) of such work; and (b) the object code compiled
from such Source Code and originally made available by Licensor under this
License.

1.11 "Personal Use" means use of Covered Code by an individual solely for his or
her personal, private and non-commercial purposes. An individual's use of
Covered Code in his or her capacity as an officer, employee, member, independent
contractor or agent of a corporation, business or organization (commercial or
non-commercial) does not qualify as Personal Use.

1.12 "Source Code" means the human readable form of a program or other work that
is suitable for making modifications to it, including all modules it contains,
plus any associated interface definition files, scripts used to control
compilation and installation of an executable (object code).

1.13 "You" or "Your" means an individual or a legal entity exercising rights
under this License. For legal entities, "You" or "Your" includes any entity
which controls, is controlled by, or is under common control with, You, where
"control" means (a) the power, direct or indirect, to cause the direction or
management of such entity, whether by contract or otherwise, or (b) ownership of
fifty percent (50%) or more of the outstanding shares or beneficial ownership of
such entity.

2. Permitted Uses; Conditions & Restrictions. Subject to the terms and
conditions of this License, Licensor hereby grants You, effective on the date
You accept this License (via downloading or using Covered Code or otherwise
indicating your acceptance of this License), a worldwide, royalty-free,
non-exclusive copyright license, to the extent of Licensor's copyrights cover
the Original Code, to do the following:

2.1 You may reproduce, display, perform, modify and Deploy Covered Code,
provided that in each instance:

(a) You must retain and reproduce in all copies of Original Code the copyright
and other proprietary notices and disclaimers of Licensor as they appear in the
Original Code, and keep intact all notices in the Original Code that refer to
this License;

(b) You must include a copy of this License with every copy of Source Code of
Covered Code and documentation You distribute, and You may not offer or impose
any terms on such Source Code that alter or restrict this License or the
recipients' rights hereunder, except as permitted under Section 6;

(c) You must duplicate, to the extent it does not already exist, the notice in
Exhibit A in each file of the Source Code of all Your Modifications, and cause
the modified files to carry prominent notices stating that You changed the files
and the date of any change;

(d) You must make Source Code of all Your Externally Deployed Modifications
publicly available under the terms of this License, including the license grants
set forth in Section 3 below, for as long as you Deploy the Covered Code or
twelve (12) months from the date of initial Deployment, whichever is longer. You
should preferably distribute the Source Code of Your Deployed Modifications
electronically (e.g. download from a web site); and

(e) if You Deploy Covered Code in object code, executable form only, You must
include a prominent notice, in the code itself as well as in related
documentation, stating that Source Code of the Covered Code is available under
the terms of this License with information on how and where to obtain such
Source Code. You must also include the Object Code Notice set forth in Exhibit A
in the "about" box or other appropriate place where other copyright notices are
placed, including any packaging materials.

2.2 You expressly acknowledge and agree that although Licensor and each
Contributor grants the licenses to their respective portions of the Covered Code
set forth herein, no assurances are provided by Licensor or any Contributor that
the Covered Code does not infringe the patent or other intellectual property
rights of any other entity. Licensor and each Contributor disclaim any liability
to You for claims brought by any other entity based on infringement of
intellectual property rights or otherwise. As a condition to exercising the
rights and licenses granted hereunder, You hereby assume sole responsibility to
secure any other intellectual property rights needed, if any. For example, if a
third party patent license is required to allow You to make, use, sell, import
or offer for sale the Covered Code, it is Your responsibility to acquire such
license(s).

2.3 Subject to the terms and conditions of this License, Licensor hereby grants
You, effective on the date You accept this License (via downloading or using
Covered Code or otherwise indicating your acceptance of this License), a
worldwide, royalty-free, perpetual, non-exclusive patent license under
Licensor's Applicable Patent Rights to make, use, sell, offer for sale and
import the Covered Code, provided that in each instance you comply with the
terms of this License.

3. Your Grants. In consideration of, and as a condition to, the licenses granted
to You under this License:

(a) You grant to Licensor and all third parties a non-exclusive, perpetual,
irrevocable, royalty free license under Your Applicable Patent Rights and other
intellectual property rights owned or controlled by You, to make, sell, offer
for sale, use, import, reproduce, display, perform, modify, distribute and
Deploy Your Modifications of the same scope and extent as Licensor's licenses
under Sections 2.1 and 2.2; and

(b) You grant to Licensor and its subsidiaries a non-exclusive, worldwide,
royalty-free, perpetual and irrevocable license, under Your Applicable Patent
Rights and other intellectual property rights owned or controlled by You, to
make, use, sell, offer for sale, import, reproduce, display, perform,
distribute, modify or have modified (for Licensor and/or its subsidiaries),
sublicense and distribute Your Modifications, in any form and for any purpose,
through multiple tiers of distribution.

(c) You agree not use any information derived from Your use and review of the
Covered Code, including but not limited to any algorithms or inventions that may
be contained in the Covered Code, for the purpose of asserting any of Your
patent rights, or assisting a third party to assert any of its patent rights,
against Licensor or any Contributor.

4. Derivative Works. You may create a Derivative Work by combining Covered Code
with other code not otherwise governed by the terms of this License and
distribute the Derivative Work as an integrated product. In each such instance,
You must make sure the requirements of this License are fulfilled for the
Covered Code or any portion thereof, including all Modifications.

4.1 You must cause any Derivative Work that you distribute, publish or
Externally Deploy, that in whole or in part contains or is derived from the
Covered Code or any part thereof, to be licensed as a whole at no charge to all
third parties under the terms of this License and no other license except as
provided in Section 4.2. You also must make Source Code available for the
Derivative Work under the same terms as Modifications, described in Sections 2
and 3, above.

4.2 Compatible Source Licenses. Software modules that have been independently
developed without any use of Covered Code and which contain no portion of the
Covered Code, Modifications or other Derivative Works, but are used or combined
in any way with the Covered Code or any Derivative Work to form a larger
Derivative Work, are exempt from the conditions described in Section 4.1 but
only to the extent that: the software module, including any software that is
linked to, integrated with, or part of the same applications as, the software
module by any method must be wholly subject to one of the Compatible Source
Licenses. Notwithstanding the foregoing, all Covered Code must be subject to the
terms of this License. Thus, the entire Derivative Work must be licensed under a
combination of the RPSL (for Covered Code) and a Compatible Source License for
any independently developed software modules within the Derivative Work. The
foregoing requirement applies even if the Compatible Source License would
ordinarily allow the software module to link with, or form larger works with,
other software that is not subject to the Compatible Source License. For
example, although the Mozilla Public License v1.1 allows Mozilla code to be
combined with proprietary software that is not subject to the MPL, if
MPL-licensed code is used with Covered Code the MPL-licensed code could not be
combined or linked with any code not governed by the MPL. The general intent of
this section 4.2 is to enable use of Covered Code with applications that are
wholly subject to an acceptable open source license. You are responsible for
determining whether your use of software with Covered Code is allowed under Your
license to such software.

4.3 Mere aggregation of another work not based on the Covered Code with the
Covered Code (or with a work based on the Covered Code) on a volume of a storage
or distribution medium does not bring the other work under the scope of this
License. If You deliver the Covered Code for combination and/or integration with
an application previously provided by You (for example, via automatic updating
technology), such combination and/or integration constitutes a Derivative Work
subject to the terms of this License.

5. Exclusions From License Grant. Nothing in this License shall be deemed to
grant any rights to trademarks, copyrights, patents, trade secrets or any other
intellectual property of Licensor or any Contributor except as expressly stated
herein. No right is granted to the trademarks of Licensor or any Contributor
even if such marks are included in the Covered Code. Nothing in this License
shall be interpreted to prohibit Licensor from licensing under different terms
from this License any code that Licensor otherwise would have a right to
license. Modifications, Derivative Works and/or any use or combination of
Covered Code with other technology provided by Licensor or third parties may
require additional patent licenses from Licensor which Licensor may grant in its
sole discretion. No patent license is granted separate from the Original Code or
combinations of the Original Code with other software or hardware.

5.1. Trademarks. This License does not grant any rights to use the trademarks or
trade names owned by Licensor ("Licensor Marks" defined in Exhibit C) or to any
trademark or trade name belonging to any Contributor. No Licensor Marks may be
used to endorse or promote products derived from the Original Code other than as
permitted by the Licensor Trademark Policy defined in Exhibit C.

6. Additional Terms. You may choose to offer, and to charge a fee for, warranty,
support, indemnity or liability obligations and/or other rights consistent with
the scope of the license granted herein ("Additional Terms") to one or more
recipients of Covered Code. However, You may do so only on Your own behalf and
as Your sole responsibility, and not on behalf of Licensor or any Contributor.
You must obtain the recipient's agreement that any such Additional Terms are
offered by You alone, and You hereby agree to indemnify, defend and hold
Licensor and every Contributor harmless for any liability incurred by or claims
asserted against Licensor or such Contributor by reason of any such Additional
Terms.

7. Versions of the License. Licensor may publish revised and/or new versions of
this License from time to time. Each version will be given a distinguishing
version number. Once Original Code has been published under a particular version
of this License, You may continue to use it under the terms of that version. You
may also choose to use such Original Code under the terms of any subsequent
version of this License published by Licensor. No one other than Licensor has
the right to modify the terms applicable to Covered Code created under this
License.

8. NO WARRANTY OR SUPPORT. The Covered Code may contain in whole or in part
pre-release, untested, or not fully tested works. The Covered Code may contain
errors that could cause failures or loss of data, and may be incomplete or
contain inaccuracies. You expressly acknowledge and agree that use of the
Covered Code, or any portion thereof, is at Your sole and entire risk. THE
COVERED CODE IS PROVIDED "AS IS" AND WITHOUT WARRANTY, UPGRADES OR SUPPORT OF
ANY KIND AND LICENSOR AND LICENSOR'S LICENSOR(S) (COLLECTIVELY REFERRED TO AS
"LICENSOR" FOR THE PURPOSES OF SECTIONS 8 AND 9) AND ALL CONTRIBUTORS EXPRESSLY
DISCLAIM ALL WARRANTIES AND/OR CONDITIONS, EXPRESS OR IMPLIED, INCLUDING, BUT
NOT LIMITED TO, THE IMPLIED WARRANTIES AND/OR CONDITIONS OF MERCHANTABILITY, OF
SATISFACTORY QUALITY, OF FITNESS FOR A PARTICULAR PURPOSE, OF ACCURACY, OF QUIET
ENJOYMENT, AND NONINFRINGEMENT OF THIRD PARTY RIGHTS. LICENSOR AND EACH
CONTRIBUTOR DOES NOT WARRANT AGAINST INTERFERENCE WITH YOUR ENJOYMENT OF THE
COVERED CODE, THAT THE FUNCTIONS CONTAINED IN THE COVERED CODE WILL MEET YOUR
REQUIREMENTS, THAT THE OPERATION OF THE COVERED CODE WILL BE UNINTERRUPTED OR
ERROR-FREE, OR THAT DEFECTS IN THE COVERED CODE WILL BE CORRECTED. NO ORAL OR
WRITTEN DOCUMENTATION, INFORMATION OR ADVICE GIVEN BY LICENSOR, A LICENSOR
AUTHORIZED REPRESENTATIVE OR ANY CONTRIBUTOR SHALL CREATE A WARRANTY. You
acknowledge that the Covered Code is not intended for use in high risk
activities, including, but not limited to, the design, construction, operation
or maintenance of nuclear facilities, aircraft navigation, aircraft
communication systems, or air traffic control machines in which case the failure
of the Covered Code could lead to death, personal injury, or severe physical or
environmental damage. Licensor disclaims any express or implied warranty of
fitness for such uses.

9. LIMITATION OF LIABILITY. TO THE EXTENT NOT PROHIBITED BY LAW, IN NO EVENT
SHALL LICENSOR OR ANY CONTRIBUTOR BE LIABLE FOR ANY INCIDENTAL, SPECIAL,
INDIRECT OR CONSEQUENTIAL DAMAGES ARISING OUT OF OR RELATING TO THIS LICENSE OR
YOUR USE OR INABILITY TO USE THE COVERED CODE, OR ANY PORTION THEREOF, WHETHER
UNDER A THEORY OF CONTRACT, WARRANTY, TORT (INCLUDING NEGLIGENCE OR STRICT
LIABILITY), PRODUCTS LIABILITY OR OTHERWISE, EVEN IF LICENSOR OR SUCH
CONTRIBUTOR HAS BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGES AND
NOTWITHSTANDING THE FAILURE OF ESSENTIAL PURPOSE OF ANY REMEDY. SOME
JURISDICTIONS DO NOT ALLOW THE LIMITATION OF LIABILITY OF INCIDENTAL OR
CONSEQUENTIAL DAMAGES, SO THIS LIMITATION MAY NOT APPLY TO YOU. In no event
shall Licensor's total liability to You for all damages (other than as may be
required by applicable law) under this License exceed the amount of ten dollars
($10.00).

10. Ownership. Subject to the licenses granted under this License, each
Contributor retains all rights, title and interest in and to any Modifications
made by such Contributor. Licensor retains all rights, title and interest in and
to the Original Code and any Modifications made by or on behalf of Licensor
("Licensor Modifications"), and such Licensor Modifications will not be
automatically subject to this License. Licensor may, at its sole discretion,
choose to license such Licensor Modifications under this License, or on
different terms from those contained in this License or may choose not to
license them at all.

11. Termination. 

11.1 Term and Termination. The term of this License is perpetual unless
terminated as provided below. This License and the rights granted hereunder will
terminate:

(a) automatically without notice from Licensor if You fail to comply with any
term(s) of this License and fail to cure such breach within 30 days of becoming
aware of such breach;

(b) immediately in the event of the circumstances described in Section 12.5(b);
or

(c) automatically without notice from Licensor if You, at any time during the
term of this License, commence an action for patent infringement against
Licensor (including by cross-claim or counter claim in a lawsuit);

(d) upon written notice from Licensor if You, at any time during the term of
this License, commence an action for patent infringement against any third party
alleging that the Covered Code itself (excluding combinations with other
software or hardware) infringes any patent (including by cross-claim or counter
claim in a lawsuit).

11.2 Effect of Termination. Upon termination, You agree to immediately stop any
further use, reproduction, modification, sublicensing and distribution of the
Covered Code and to destroy all copies of the Covered Code that are in your
possession or control. All sublicenses to the Covered Code which have been
properly granted prior to termination shall survive any termination of this
License. Provisions which, by their nature, should remain in effect beyond the
termination of this License shall survive, including but not limited to Sections
3, 5, 8, 9, 10, 11, 12.2 and 13. No party will be liable to any other for
compensation, indemnity or damages of any sort solely as a result of terminating
this License in accordance with its terms, and termination of this License will
be without prejudice to any other right or remedy of any party.

12. Miscellaneous.

12.1 Government End Users. The Covered Code is a "commercial item" as defined in
FAR 2.101. Government software and technical data rights in the Covered Code
include only those rights customarily provided to the public as defined in this
License. This customary commercial license in technical data and software is
provided in accordance with FAR 12.211 (Technical Data) and 12.212 (Computer
Software) and, for Department of Defense purchases, DFAR 252.227-7015 (Technical
Data -- Commercial Items) and 227.7202-3 (Rights in Commercial Computer Software
or Computer Software Documentation). Accordingly, all U.S. Government End Users
acquire Covered Code with only those rights set forth herein.

12.2 Relationship of Parties. This License will not be construed as creating an
agency, partnership, joint venture or any other form of legal association
between or among You, Licensor or any Contributor, and You will not represent to
the contrary, whether expressly, by implication, appearance or otherwise.

12.3 Independent Development. Nothing in this License will impair Licensor's
right to acquire, license, develop, have others develop for it, market and/or
distribute technology or products that perform the same or similar functions as,
or otherwise compete with, Modifications, Derivative Works, technology or
products that You may develop, produce, market or distribute.

12.4 Waiver; Construction. Failure by Licensor or any Contributor to enforce any
provision of this License will not be deemed a waiver of future enforcement of
that or any other provision. Any law or regulation which provides that the
language of a contract shall be construed against the drafter will not apply to
this License.

12.5 Severability. (a) If for any reason a court of competent jurisdiction finds
any provision of this License, or portion thereof, to be unenforceable, that
provision of the License will be enforced to the maximum extent permissible so
as to effect the economic benefits and intent of the parties, and the remainder
of this License will continue in full force and effect. (b) Notwithstanding the
foregoing, if applicable law prohibits or restricts You from fully and/or
specifically complying with Sections 2 and/or 3 or prevents the enforceability
of either of those Sections, this License will immediately terminate and You
must immediately discontinue any use of the Covered Code and destroy all copies
of it that are in your possession or control.

12.6 Dispute Resolution. Any litigation or other dispute resolution between You
and Licensor relating to this License shall take place in the Seattle,
Washington, and You and Licensor hereby consent to the personal jurisdiction of,
and venue in, the state and federal courts within that District with respect to
this License. The application of the United Nations Convention on Contracts for
the International Sale of Goods is expressly excluded.

12.7 Export/Import Laws. This software is subject to all export and import laws
and restrictions and regulations of the country in which you receive the Covered
Code and You are solely responsible for ensuring that You do not export,
re-export or import the Covered Code or any direct product thereof in violation
of any such restrictions, laws or regulations, or without all necessary
authorizations.

12.8 Entire Agreement; Governing Law. This License constitutes the entire
agreement between the parties with respect to the subject matter hereof. This
License shall be governed by the laws of the United States and the State of
Washington.

Where You are located in the province of Quebec, Canada, the following clause
applies: The parties hereby confirm that they have requested that this License
and all related documents be drafted in English. Les parties ont exig&eacute;
que le pr&eacute;sent contrat et tous les documents connexes soient
r&eacute;dig&eacute;s en anglais.

								EXHIBIT A.  

"Copyright &copy; 1995-2002
RealNetworks, Inc. and/or its licensors. All Rights Reserved.

The contents of this file, and the files included with this file, are subject to
the current version of the RealNetworks Public Source License Version 1.0 (the
"RPSL") available at https://www.helixcommunity.org/content/rpsl unless you have
licensed the file under the RealNetworks Community Source License Version 1.0
(the "RCSL") available at https://www.helixcommunity.org/content/rcsl, in which
case the RCSL will apply. You may also obtain the license terms directly from
RealNetworks. You may not use this file except in compliance with the RPSL or,
if you have a valid RCSL with RealNetworks applicable to this file, the RCSL.
Please see the applicable RPSL or RCSL for the rights, obligations and
limitations governing use of the contents of the file.

This file is part of the Helix DNA Technology. RealNetworks is the developer of
the Original code and owns the copyrights in the portions it created.

This file, and the files included with this file, is distributed and made
available on an 'AS IS' basis, WITHOUT WARRANTY OF ANY KIND, EITHER EXPRESS OR
IMPLIED, AND REALNETWORKS HEREBY DISCLAIMS ALL SUCH WARRANTIES, INCLUDING
WITHOUT LIMITATION, ANY WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR
PURPOSE, QUIET ENJOYMENT OR NON-INFRINGEMENT.

Contributor(s): ____________________________________ 

Technology Compatibility Kit Test
Suite(s) Location (if licensed under the RCSL): ______________________________ 

Object Code Notice: Helix DNA Client technology included. Copyright (c)
RealNetworks, Inc., 1995-2002. All rights reserved.


								EXHIBIT B 

Compatible Source Licenses for the RealNetworks Public Source License. The
following list applies to the most recent version of the license as of October
25, 2002, unless otherwise indicated.

* Academic Free License
* Apache Software License
* Apple Public Source License
* Artistic license
* Attribution Assurance Licenses
* BSD license
* Common Public License (1)
* Eiffel Forum License
* GNU General Public License (GPL) (1)
* GNU Library or "Lesser" General Public License (LGPL) (1)
* IBM Public License
* Intel Open Source License
* Jabber Open Source License
* MIT license
* MITRE Collaborative Virtual Workspace License (CVW License)
* Motosoto License
* Mozilla Public License 1.0 (MPL)
* Mozilla Public License 1.1 (MPL)
* Nokia Open Source License
* Open Group Test Suite License
* Python Software Foundation License
* Ricoh Source Code Public License
* Sun Industry Standards Source License (SISSL)
* Sun Public License
* University of Illinois/NCSA Open Source License
* Vovida Software License v. 1.0
* W3C License
* X.Net License
* Zope Public License
* zlib/libpng license

(1) Note: because this license contains certain reciprocal licensing terms that
purport to extend to independently developed code, You may be prohibited under
the terms of this otherwise compatible license from using code licensed under
its terms with Covered Code because Covered Code may only be licensed under the
RealNetworks Public Source License. Any attempt to apply non RPSL license terms,
including without limitation the GPL, to Covered Code is expressly forbidden.
You are responsible for ensuring that Your use of Compatible Source Licensed
code does not violate either the RPSL or the Compatible Source License.

The latest version of this list can be found at:
https://www.helixcommunity.org/content/complicense

								EXHIBIT C 

RealNetworks' Trademark policy.  

RealNetworks defines the following trademarks collectively as "Licensor
Trademarks": "RealNetworks", "RealPlayer", "RealJukebox", "RealSystem",
"RealAudio", "RealVideo", "RealOne Player", "RealMedia", "Helix" or any other
trademarks or trade names belonging to RealNetworks.

RealNetworks "Licensor Trademark Policy" forbids any use of Licensor Trademarks
except as permitted by and in strict compliance at all times with RealNetworks'
third party trademark usage guidelines which are posted at
http://www.realnetworks.com/info/helixlogo.html.

~~~~~~~~~~

### Licence text J - RealNetworks Community Source License Version 1.0 (RCSL)

Text copied from: ESP8266Audio 2.2.0, file src/libhelix-mp3/RCSL.txt

~~~~~~~~~~
The RCSL is made up of a base agreement and a few Attachments.

For Research and Development use, you agree to the terms of the
RCSL R&D License (base RCSL and Attachments A, B, and C) 

For Commercial Use (either distribution or internal commercial
deployment) of the Helix DNA with or without support for RealNetworks'
RealAudio and RealVideo Add-on Technology, you agree to the
terms of the same RCSL R&D license
and execute one or more additional Commercial Use License attachments
<see http://www.helixcommunity.org/content/rcsl-attachments>.

------------------------------------------------------------------------


    REALNETWORKS COMMUNITY SOURCE LICENSE

Version 1.2 (Rev. Date: January 22, 2003).


  RECITALS

Original Contributor has developed Specifications, Source Code
implementations and Executables of certain Technology; and

Original Contributor desires to license the Technology to a large
community to facilitate research, innovation and product development
while maintaining compatibility of such products with the Technology as
delivered by Original Contributor; and

Original Contributor desires to license certain Trademarks for the
purpose of branding products that are compatible with the relevant
Technology delivered by Original Contributor; and

You desire to license the Technology and possibly certain Trademarks
from Original Contributor on the terms and conditions specified in this
License.

In consideration for the mutual covenants contained herein, You and
Original Contributor agree as follows:


  AGREEMENT

*1. Introduction.*

The RealNetworks Community Source License ("RCSL") and effective
attachments ("License") may include five distinct licenses:

i) Research Use license -- License plus Attachments A, B and C only.

ii) Commercial Use and Trademark License, which may be for Internal
Deployment Use or external distribution, or both -- License plus
Attachments A, B, C, and D.

iii) Technology Compatibility Kit (TCK) license -- Attachment C.

iv) Add-On Technology License (Executable) Commercial Use License
-Attachment F.

v) Add-On Technology Source Code Porting and Optimization
License-Attachment G.

The Research Use license is effective when You click and accept this
License. The TCK is effective when You click and accept this License,
unless otherwise specified in the TCK attachments. The Commercial Use
and Trademark, Add-On Technology License, and the Add-On Technology
Source Code Porting and Optimization licenses must each be signed by You
and Original Contributor to become effective. Once effective, these
licenses and the associated requirements and responsibilities are
cumulative. Capitalized terms used in this License are defined in the
Glossary.

*2. License Grants.*

2.1 Original Contributor Grant.

Subject to Your compliance with Sections 3, 8.10 and Attachment A of
this License, Original Contributor grants to You a worldwide,
royalty-free, non-exclusive license, to the extent of Original
Contributor's Intellectual Property Rights covering the Original Code,
Upgraded Code and Specifications, to do the following:

(a) Research Use License:

(i) use, reproduce and modify the Original Code, Upgraded Code and
Specifications to create Modifications and Reformatted Specifications
for Research Use by You;

(ii) publish and display Original Code, Upgraded Code and Specifications
with, or as part of Modifications, as permitted under Section 3.1(b) below;

(iii) reproduce and distribute copies of Original Code and Upgraded Code
to Licensees and students for Research Use by You;

(iv) compile, reproduce and distribute Original Code and Upgraded Code
in Executable form, and Reformatted Specifications to anyone for
Research Use by You.

(b) Other than the licenses expressly granted in this License, Original
Contributor retains all right, title, and interest in Original Code and
Upgraded Code and Specifications.

2.2 Your Grants.

(a) To Other Licensees. You hereby grant to each Licensee a license to
Your Error Corrections and Shared Modifications, of the same scope and
extent as Original Contributor's licenses under Section 2.1 a) above
relative to Research Use and Attachment D relative to Commercial Use.

(b) To Original Contributor. You hereby grant to Original Contributor a
worldwide, royalty-free, non-exclusive, perpetual and irrevocable
license, to the extent of Your Intellectual Property Rights covering
Your Error Corrections, Shared Modifications and Reformatted
Specifications, to use, reproduce, modify, display and distribute Your
Error Corrections, Shared Modifications and Reformatted Specifications,
in any form, including the right to sublicense such rights through
multiple tiers of distribution.

(c) Other than the licenses expressly granted in Sections 2.2(a) and (b)
above, and the restrictions set forth in Section 3.1(d)(iv) below, You
retain all right, title, and interest in Your Error Corrections, Shared
Modifications and Reformatted Specifications.

2.3 Contributor Modifications.

You may use, reproduce, modify, display and distribute Contributor Error
Corrections, Shared Modifications and Reformatted Specifications,
obtained by You under this License, to the same scope and extent as with
Original Code, Upgraded Code and Specifications.

2.4 Subcontracting.

You may deliver the Source Code of Covered Code to other Licensees
having at least a Research Use license, for the sole purpose of
furnishing development services to You in connection with Your rights
granted in this License. All such Licensees must execute appropriate
documents with respect to such work consistent with the terms of this
License, and acknowledging their work-made-for-hire status or assigning
exclusive right to the work product and associated Intellectual Property
Rights to You.

*3. Requirements and Responsibilities*.

3.1 Research Use License.

As a condition of exercising the rights granted under Section 2.1(a)
above, You agree to comply with the following:

(a) Your Contribution to the Community. All Error Corrections and Shared
Modifications which You create or contribute to are automatically
subject to the licenses granted under Section 2.2 above. You are
encouraged to license all of Your other Modifications under Section 2.2
as Shared Modifications, but are not required to do so. You agree to
notify Original Contributor of any errors in the Specification.

(b) Source Code Availability. You agree to provide all Your Error
Corrections to Original Contributor as soon as reasonably practicable
and, in any event, prior to Internal Deployment Use or Commercial Use,
if applicable. Original Contributor may, at its discretion, post Source
Code for Your Error Corrections and Shared Modifications on the
Community Webserver. You may also post Error Corrections and Shared
Modifications on a web-server of Your choice; provided, that You must
take reasonable precautions to ensure that only Licensees have access to
such Error Corrections and Shared Modifications. Such precautions shall
include, without limitation, a password protection scheme limited to
Licensees and a click-on, download certification of Licensee status
required of those attempting to download from the server. An example of
an acceptable certification is attached as Attachment A-2.

(c) Notices. All Error Corrections and Shared Modifications You create
or contribute to must include a file documenting the additions and
changes You made and the date of such additions and changes. You must
also include the notice set forth in Attachment A-1 in the file header.
If it is not possible to put the notice in a particular Source Code file
due to its structure, then You must include the notice in a location
(such as a relevant directory file), where a recipient would be most
likely to look for such a notice.

(d) Redistribution.

(i) Source. Covered Code may be distributed in Source Code form only to
another Licensee (except for students as provided below). You may not
offer or impose any terms on any Covered Code that alter the rights,
requirements, or responsibilities of such Licensee. You may distribute
Covered Code to students for use in connection with their course work
and research projects undertaken at accredited educational institutions.
Such students need not be Licensees, but must be given a copy of the
notice set forth in Attachment A-3 and such notice must also be included
in a file header or prominent location in the Source Code made available
to such students.

(ii) Executable. You may distribute Executable version(s) of Covered
Code to Licensees and other third parties only for the purpose of
evaluation and comment in connection with Research Use by You and under
a license of Your choice, but which limits use of such Executable
version(s) of Covered Code only to that purpose.

(iii) Modified Class, Interface and Package Naming. In connection with
Research Use by You only, You may use Original Contributor's class,
Interface and package names only to accurately reference or invoke the
Source Code files You modify. Original Contributor grants to You a
limited license to the extent necessary for such purposes.

(iv) You expressly agree that any distribution, in whole or in part, of
Modifications developed by You shall only be done pursuant to the terms
and conditions of this License.

(e) Extensions.

(i) Covered Code. You may not include any Source Code of Community Code
in any Extensions. You may include the compiled Header Files of
Community Code in an Extension provided that Your use of the Covered
Code, including Heading Files, complies with the Commercial Use License,
the TCK and all other terms of this License.

(ii) Publication. No later than the date on which You first distribute
such Extension for Commercial Use, You must publish to the industry, on
a non-confidential basis and free of all copyright restrictions with
respect to reproduction and use, an accurate and current specification
for any Extension. In addition, You must make available an appropriate
test suite, pursuant to the same rights as the specification,
sufficiently detailed to allow any third party reasonably skilled in the
technology to produce implementations of the Extension compatible with
the specification. Such test suites must be made available as soon as
reasonably practicable but, in no event, later than ninety (90) days
after Your first Commercial Use of the Extension. You must use
reasonable efforts to promptly clarify and correct the specification and
the test suite upon written request by Original Contributor.

(iii) Open. You agree to refrain from enforcing any Intellectual
Property Rights You may have covering any interface(s) of Your
Extension, which would prevent the implementation of such interface(s)
by Original Contributor or any Licensee. This obligation does not
prevent You from enforcing any Intellectual Property Right You have that
would otherwise be infringed by an implementation of Your Extension.

(iv) Interface Modifications and Naming. You may not modify or add to
the GUID space * * "xxxxxxxx-0901-11d1-8B06-00A024406D59" or any other
GUID space designated by Original Contributor. You may not modify any
Interface prefix provided with the Covered Code or any other prefix
designated by Original Contributor.* *

* *

(f) You agree that any Specifications provided to You by Original
Contributor are confidential and proprietary information of Original
Contributor. You must maintain the confidentiality of the Specifications
and may not disclose them to any third party without Original
Contributor's prior written consent. You may only use the Specifications
under the terms of this License and only for the purpose of implementing
the terms of this License with respect to Covered Code. You agree not
use, copy or distribute any such Specifications except as provided in
writing by Original Contributor.

3.2 Commercial Use License.

You may not make Commercial Use of any Covered Code unless You and
Original Contributor have executed a copy of the Commercial Use and
Trademark License attached as Attachment D.

*4. Versions of the License.*

4.1 License Versions.

Original Contributor may publish revised versions of the License from
time to time. Each version will be given a distinguishing version number.

4.2 Effect.

Once a particular version of Covered Code has been provided under a
version of the License, You may always continue to use such Covered Code
under the terms of that version of the License. You may also choose to
use such Covered Code under the terms of any subsequent version of the
License. No one other than Original Contributor has the right to
promulgate License versions.

4.3 Multiple-Licensed Code.

Original Contributor may designate portions of the Covered Code as
"Multiple-Licensed." "Multiple-Licensed" means that the Original
Contributor permits You to utilize those designated portions of the
Covered Code under Your choice of this License or the alternative
license(s), if any, specified by the Original Contributor in an
Attachment to this License.

*5. Disclaimer of Warranty.*

5.1 COVERED CODE PROVIDED AS IS.

COVERED CODE IS PROVIDED UNDER THIS LICENSE "AS IS," WITHOUT WARRANTY OF
ANY KIND, EITHER EXPRESS OR IMPLIED, INCLUDING, WITHOUT LIMITATION,
WARRANTIES THAT THE COVERED CODE IS FREE OF DEFECTS, MERCHANTABLE, FIT
FOR A PARTICULAR PURPOSE OR NON-INFRINGING. YOU AGREE TO BEAR THE ENTIRE
RISK IN CONNECTION WITH YOUR USE AND DISTRIBUTION OF COVERED CODE UNDER
THIS LICENSE. THIS DISCLAIMER OF WARRANTY CONSTITUTES AN ESSENTIAL PART
OF THIS LICENSE. NO USE OF ANY COVERED CODE IS AUTHORIZED HEREUNDER
EXCEPT SUBJECT TO THIS DISCLAIMER.

5.2 Not Designed for High Risk Activities.

You acknowledge that Original Code, Upgraded Code and Specifications are
not designed or intended for use in high risk activities including, but
not limited to: (i) on-line control of aircraft, air traffic, aircraft
navigation or aircraft communications; or (ii) in the design,
construction, operation or maintenance of any nuclear facility. Original
Contributor disclaims any express or implied warranty of fitness for
such uses.

*6. Termination.*

6.1 By You.

You may terminate this Research Use license at anytime by providing
written notice to Original Contributor.

6.2 By Original Contributor.

This License and the rights granted hereunder will terminate:

(i) automatically if You fail to comply with the terms of this License
and fail to cure such breach within 30 days of receipt of written notice
of the breach;

(ii) immediately in the event of circumstances specified in Sections 7.1
and 8.4; or

(iii) at Original Contributor's discretion upon any action initiated by
You (including by cross-claim or counter claim) alleging that use or
distribution by Original Contributor or any Licensee, of Original Code,
Upgraded Code, Error Corrections, Shared Modifications or Specifications
infringe a patent owned or controlled by You.

6.3 Effective of Termination.

Upon termination, You agree to discontinue use of and destroy all copies
of Covered Code in Your possession. All sublicenses to the Covered Code
which You have properly granted shall survive any termination of this
License. Provisions that, by their nature, should remain in effect
beyond the termination of this License shall survive including, without
limitation, Sections 2.2, 3, 5, 7 and 8.

6.4 No Compensation.

Each party waives and releases the other from any claim to compensation
or indemnity for permitted or lawful termination of the business
relationship established by this License.

*7. Liability.*

7.1 Infringement. Should any of the Original Code, Upgraded Code, TCK or
Specifications ("Materials") become the subject of a claim of
infringement, Original Contributor may, at its sole option, (i) attempt
to procure the rights necessary for You to continue using the Materials,
(ii) modify the Materials so that they are no longer infringing, or
(iii) terminate Your right to use the Materials, immediately upon
written notice, and refund to You the amount, if any, having then
actually been paid by You to Original Contributor for the Original Code,
Upgraded Code and TCK, depreciated on a straight line, five year basis.

7.2 LIMITATION OF LIABILITY. TO THE FULL EXTENT ALLOWED BY APPLICABLE
LAW, ORIGINAL CONTRIBUTOR'S LIABILITY TO YOU FOR CLAIMS RELATING TO THIS
LICENSE, WHETHER FOR BREACH OR IN TORT, SHALL BE LIMITED TO ONE HUNDRED
PERCENT (100%) OF THE AMOUNT HAVING THEN ACTUALLY BEEN PAID BY YOU TO
ORIGINAL CONTRIBUTOR FOR ALL COPIES LICENSED HEREUNDER OF THE PARTICULAR
ITEMS GIVING RISE TO SUCH CLAIM, IF ANY, DURING THE TWELVE MONTHS
PRECEDING THE CLAIMED BREACH. IN NO EVENT WILL YOU (RELATIVE TO YOUR
SHARED MODIFICATIONS OR ERROR CORRECTIONS) OR ORIGINAL CONTRIBUTOR BE
LIABLE FOR ANY INDIRECT, PUNITIVE, SPECIAL, INCIDENTAL OR CONSEQUENTIAL
DAMAGES IN CONNECTION WITH OR RISING OUT OF THIS LICENSE (INCLUDING,
WITHOUT LIMITATION, LOSS OF PROFITS, USE, DATA, OR OTHER ECONOMIC
ADVANTAGE), HOWEVER IT ARISES AND ON ANY THEORY OF LIABILITY, WHETHER IN
AN ACTION FOR CONTRACT, STRICT LIABILITY OR TORT (INCLUDING NEGLIGENCE)
OR OTHERWISE, WHETHER OR NOT YOU OR ORIGINAL CONTRIBUTOR HAS BEEN
ADVISED OF THE POSSIBILITY OF SUCH DAMAGE AND NOTWITHSTANDING THE
FAILURE OF ESSENTIAL PURPOSE OF ANY REMEDY.

*8. Miscellaneous.*

8.1 Trademark.

You shall not use any Trademark unless You and Original Contributor
execute a copy of the Commercial Use and Trademark License Agreement
attached hereto as Attachment D. Except as expressly provided in the
License, You are granted no right, title or license to, or interest in,
any Trademarks. Whether or not You and Original Contributor enter into
the Trademark License, You agree not to (i) challenge Original
Contributor's ownership or use of Trademarks; (ii) attempt to register
any Trademarks, or any mark or logo substantially similar thereto; or
(iii) incorporate any Trademarks into Your own trademarks, product
names, service marks, company names, or domain names.

8.2 Integration.

This License represents the complete agreement concerning the subject
matter hereof.

8.3 Assignment.

Original Contributor may assign this License, and its rights and
obligations hereunder, in its sole discretion. You may assign the
Research Use portions of this License and the TCK license to a third
party upon prior written notice to Original Contributor (which may be
provided electronically via the Community Web-Server). You may not
assign the Commercial Use and Trademark license, the Add-On Technology
License, or the Add-On Technology Source Code Porting License, including
by way of merger (regardless of whether You are the surviving entity) or
acquisition, without Original Contributor's prior written consent.

8.4 Severability.

If any provision of this License is held to be unenforceable, such
provision shall be reformed only to the extent necessary to make it
enforceable. Notwithstanding the foregoing, if You are prohibited by law
from fully and specifically complying with Sections 2.2 or 3, this
License will immediately terminate and You must immediately discontinue
any use of Covered Code.

8.5 Governing Law.

This License shall be governed by the laws of the United States and the
State of Washington, as applied to contracts entered into and to be
performed in Washington between Washington residents. The application of
the United Nations Convention on Contracts for the International Sale of
Goods is expressly excluded. You agree that the state and federal courts
located in Seattle, Washington have exclusive jurisdiction over any
claim relating to the License, including contract and tort claims.

8.6 Dispute Resolution.

a) Arbitration. Any dispute arising out of or relating to this License
shall be finally settled by arbitration as set out herein, except that
either party may bring any action, in a court of competent jurisdiction
(which jurisdiction shall be exclusive), with respect to any dispute
relating to such party's Intellectual Property Rights or with respect to
Your compliance with the TCK license. Arbitration shall be administered:
(i) by the American Arbitration Association (AAA), (ii) in accordance
with the rules of the United Nations Commission on International Trade
Law (UNCITRAL) (the "Rules") in effect at the time of arbitration as
modified herein; and (iii) the arbitrator will apply the substantive
laws of Washington and the United States. Judgment upon the award
rendered by the arbitrator may be entered in any court having
jurisdiction to enforce such award.

b) Arbitration language, venue and damages. All arbitration proceedings
shall be conducted in English by a single arbitrator selected in
accordance with the Rules, who must be fluent in English and be either a
retired judge or practicing attorney having at least ten (10) years
litigation experience and be reasonably familiar with the technology
matters relative to the dispute. Unless otherwise agreed, arbitration
venue shall be in Seattle, Washington. The arbitrator may award monetary
damages only and nothing shall preclude either party from seeking
provisional or emergency relief from a court of competent jurisdiction.
The arbitrator shall have no authority to award damages in excess of
those permitted in this License and any such award in excess is void.
All awards will be payable in U.S. dollars and may include, for the
prevailing party (i) pre-judgment award interest, (ii) reasonable
attorneys' fees incurred in connection with the arbitration, and (iii)
reasonable costs and expenses incurred in enforcing the award. The
arbitrator will order each party to produce identified documents and
respond to no more than twenty-five single question interrogatories.

8.7 Construction.

Any law or regulation, which provides that the language of a contract
shall be construed against the drafter, shall not apply to this License.

8.8 U.S. Government End Users.

The Covered Code is a "commercial item," as that term is defined in 48
C.F.R. 2.101 (Oct. 1995), consisting of "commercial computer software"
and "commercial computer software documentation," as such terms are used
in 48 C.F.R. 12.212 (Sept. 1995). Consistent with 48 C.F.R. 12.212 and
48 C.F.R. 227.7202-1 through 227.7202-4 (June 1995), all U.S. Government
End Users acquire Covered Code with only those rights set forth herein.
You agree to pass this notice to our licensees.

8.9 Marketing Activities.

Licensee hereby grants Original Contributor a non-exclusive,
non-transferable, limited license to use the Licensee's company name and
logo ("Licensee Marks") in any presentations, press releases, or
marketing materials solely for the purpose of identifying Licensee as a
member of the Helix Community. Licensee shall provide samples of
Licensee Marks to Original Contributor upon request by Original
Contributor. Original Contributor acknowledges that the Licensee Marks
are the trademarks of Licensee. Original Contributor shall not use the
Licensee Marks in a way that may imply that Original Contributor is an
agency or branch of Licensee. Original Contributor understands and
agrees that the use of any Licensee Marks in connection with this
Agreement shall not create any right, title or interest, in, or to the
Licensee Marks or any Licensee trademarks and that all such use and
goodwill associated with any such trademarks will inure to the benefit
of Licensee. Further the Original Contributor will stop usage of the
Licensee Marks upon Licensee's request.

8.10 Press Announcements.

You may make press announcements or other public statements regarding
this License without the prior written consent of the Original
Contributor, if Your statement is limited to announcing the licensing of
the Covered Code or the availability of Your Product and its
compatibility with the Covered Code. All other public announcements
regarding this license require the prior written consent of the Original
Contributor. Consent requests are welcome at press@helixcommunity.org.

8.11 International Use.

a) Export/Import laws. Covered Code is subject to U.S. export control
laws and may be subject to export or import regulations in other
countries. Each party agrees to comply strictly with all such laws and
regulations and acknowledges their responsibility to obtain such
licenses to export, re-export, or import as may be required. You agree
to pass these obligations to Your licensees.

b) Intellectual Property Protection. Due to limited intellectual
property protection and enforcement in certain countries, You agree not
to redistribute the Original Code, Upgraded Code, TCK and Specifications
to any country on the list of restricted countries on the Community Web
Server.

8.12 Language.

This License is in the English language only, which language shall be
controlling in all respects, and all versions of this License in any
other language shall be for accommodation only and shall not be binding
on the parties to this License. All communications and notices made or
given pursuant to this License, and all documentation and support to be
provided, unless otherwise noted, shall be in the English language.

PLEASE READ THE TERMS OF THIS LICENSE CAREFULLY. BY CLICKING ON THE
"ACCEPT" BUTTON BELOW YOU ARE ACCEPTING AND AGREEING TO THE TERMS AND
CONDITIONS OF THIS LICENSE WITH REALNETWORKS, INC. IF YOU ARE AGREEING
TO THIS LICENSE ON BEHALF OF A COMPANY, YOU REPRESENT THAT YOU ARE
AUTHORIZED TO BIND THE COMPANY TO SUCH A LICENSE. WHETHER YOU ARE ACTING
ON YOUR OWN BEHALF, OR REPRESENTING A COMPANY, YOU MUST BE OF MAJORITY
AGE AND BE OTHERWISE COMPETENT TO ENTER INTO CONTRACTS. IF YOU DO NOT
MEET THIS CRITERIA OR YOU DO NOT AGREE TO ANY OF THE TERMS AND
CONDITIONS OF THIS LICENSE, CLICK ON THE REJECT BUTTON TO EXIT.


    GLOSSARY

1. *"Added Value"* means code which:

(i) has a principal purpose which is substantially different from that
of the stand-alone Technology;

(ii) represents a significant functional and value enhancement to the
Technology;

(iii) operates in conjunction with the Technology; and

(iv) is not marketed as a technology which replaces or substitutes for
the Technology

2. "*Applicable Patent Rights*" mean: (a) in the case where Original
Contributor is the grantor of rights, claims of patents that (i) are now
or hereafter acquired, owned by or assigned to Original Contributor and
(ii) are necessarily infringed by using or making the Original Code or
Upgraded Code, including Modifications provided by Original Contributor,
alone and not in combination with other software or hardware; and (b) in
the case where Licensee is the grantor of rights, claims of patents that
(i) are now or hereafter acquired, owned by or assigned to Licensee and
(ii) are infringed (directly or indirectly) by using or making
Licensee's Modifications or Error Corrections, taken alone or in
combination with Covered Code.

3. "*Application Programming Interfaces (APIs)"* means the interfaces,
associated header files, service provider interfaces, and protocols that
enable a device, application, Operating System, or other program to
obtain services from or make requests of (or provide services in
response to requests from) other programs, and to use, benefit from, or
rely on the resources, facilities, and capabilities of the relevant
programs using the APIs. APIs includes the technical documentation
describing the APIs, the Source Code constituting the API, and any
Header Files used with the APIs.

4. "*Commercial Use*" means any use (internal or external), copying,
sublicensing or distribution (internal or external), directly or
indirectly of Covered Code by You other than Your Research Use of
Covered Code within Your business or organization or in conjunction with
other Licensees with equivalent Research Use rights. Commercial Use
includes any use of the Covered Code for direct or indirect commercial
or strategic gain, advantage or other business purpose. Any Commercial
Use requires execution of Attachment D by You and Original Contributor.

5. "*Community Code*" means the Original Code, Upgraded Code, Error
Corrections, Shared Modifications, or any combination thereof.

6. "*Community Webserver(s)"* means the webservers designated by
Original Contributor for access to the Original Code, Upgraded Code, TCK
and Specifications and for posting Error Corrections and Shared
Modifications.

7. "*Compliant Covered Code*" means Covered Code that complies with the
requirements of the TCK.

8. "*Contributor*" means each Licensee that creates or contributes to
the creation of any Error Correction or Shared Modification.

9. "*Covered Code*" means the Original Code, Upgraded Code,
Modifications, or any combination thereof.

10. "*Error Correction*" means any change made to Community Code which
conforms to the Specification and corrects the adverse effect of a
failure of Community Code to perform any function set forth in or
required by the Specifications.

11. "*Executable*" means Covered Code that has been converted from
Source Code to the preferred form for execution by a computer or digital
processor (e.g. binary form).

12. "*Extension(s)"* means any additional Interfaces developed by or for
You which: (i) are designed for use with the Technology; (ii) constitute
an API for a library of computing functions or services; and (iii) are
disclosed or otherwise made available to third party software developers
for the purpose of developing software which invokes such additional
Interfaces. The foregoing shall not apply to software developed by Your
subcontractors to be exclusively used by You.

13. "*Header File(s)"* means that portion of the Source Code that
provides the names and types of member functions, data members, class
definitions, and interface definitions necessary to implement the APIs
for the Covered Code. Header Files include, files specifically
designated by Original Contributor as Header Files. Header Files do not
include the code necessary to implement the functionality underlying the
Interface.

14. *"Helix DNA Server Technology"* means the program(s) that implement
the Helix Universal Server streaming engine for the Technology as
defined in the Specification.

15. *"Helix DNA Client Technology"* means the Covered Code that
implements the RealOne Player engine as defined in the Specification.

16. *"Helix DNA Producer Technology"* means the Covered Code that
implements the Helix Producer engine as defined in the Specification.

17. *"Helix DNA Technology"* means the Helix DNA Server Technology, the
Helix DNA Client Technology, the Helix DNA Producer Technology and other
Helix technologies designated by Original Contributor.

18. "*Intellectual Property Rights*" means worldwide statutory and
common law rights associated solely with (i) Applicable Patent Rights;
(ii) works of authorship including copyrights, copyright applications,
copyright registrations and "moral rights"; (iii) the protection of
trade and industrial secrets and confidential information; and (iv)
divisions, continuations, renewals, and re-issuances of the foregoing
now existing or acquired in the future.

19. *"Interface*" means interfaces, functions, properties, class
definitions, APIs, Header Files, GUIDs, V-Tables, and/or protocols
allowing one piece of software, firmware or hardware to communicate or
interoperate with another piece of software, firmware or hardware.

20. "*Internal Deployment Use*" means use of Compliant Covered Code
(excluding Research Use) within Your business or organization only by
Your employees and/or agents on behalf of Your business or organization,
but not to provide services, including content distribution, to third
parties, subject to execution of Attachment D by You and Original
Contributor, if required.

21. "*Licensee*" means any party that has entered into and has in effect
a version of this License with Original Contributor.

22. "*MIME type*" means a description of what type of media or other
content is in a file, including by way of example but not limited to
'audio/x-pn-realaudio-plugin.'

23. "*Modification(s)"* means (i) any addition to, deletion from and/or
change to the substance and/or structure of the Covered Code, including
Interfaces; (ii) the combination of any Covered Code and any previous
Modifications; (iii) any new file or other representation of computer
program statements that contains any portion of Covered Code; and/or
(iv) any new Source Code implementing any portion of the Specifications.

24. "*MP3 Patents*" means any patents necessary to make, use or sell
technology implementing any portion of the specification developed by
the Moving Picture Experts Group known as MPEG-1 Audio Layer-3 or MP3,
including but not limited to all past and future versions, profiles,
extensions, parts and amendments relating to the MP3 specification.

25. "*MPEG-4 Patents*" means any patents necessary to make, use or sell
technology implementing any portion of the specification developed by
the Moving Pictures Experts Group known as MPEG-4, including but not
limited to all past and future versions, profiles, extensions, parts and
amendments relating to the MPEG-4 specification.

26. "*Original Code*" means the initial Source Code for the Technology
as described on the Community Web Server.

27. "*Original Contributor*" means RealNetworks, Inc., its affiliates
and its successors and assigns.

28. "*Original Contributor MIME Type*" means the MIME registry, browser
preferences, or local file/protocol associations invoking any Helix DNA
Client-based application, including the RealOne Player, for playback of
RealAudio, RealVideo, other RealMedia MIME types or datatypes (e.g.,
.ram, .rnx, .rpm, .ra, .rm, .rp, .rt, .rf, .prx, .mpe, .rmp, .rmj, .rav,
.rjs, .rmx, .rjt, .rms), and any other Original Contributor-specific or
proprietary MIME types that Original Contributor may introduce in the
future.

29. "*Personal Use*" means use of Covered Code by an individual solely
for his or her personal, private and non-commercial purposes. An
individual's use of Covered Code in his or her capacity as an officer,
employee, member, independent contractor or agent of a corporation,
business or organization (commercial or non-commercial) does not qualify
as Personal Use.

30. "*RealMedia File Format*" means the file format designed and
developed by RealNetworks for storing multimedia data and used to store
RealAudio and RealVideo encoded streams. Valid RealMedia File Format
extensions include: .rm, .rmj, .rmc, .rmvb, .rms.

31. "*RCSL Webpage*" means the RealNetworks Community Source License
webpage located at https://www.helixcommunity.org/content/rcsl or such
other URL that Original Contributor may designate from time to time.

32. "*Reformatted Specifications*" means any revision to the
Specifications which translates or reformats the Specifications (as for
example in connection with Your documentation) but which does not alter,
subset or superset * *the functional or operational aspects of the
Specifications.

33. "*Research Use*" means use and distribution of Covered Code only for
Your Personal Use, research or development use and expressly excludes
Internal Deployment Use and Commercial Use. Research Use also includes
use of Covered Code to teach individuals how to use Covered Code.

34. "*Shared Modifications*" means Modifications that You distribute or
use for a Commercial Use, in addition to any Modifications provided by
You, at Your option, pursuant to Section 2.2, or received by You from a
Contributor pursuant to Section 2.3.

35. "*Source Code*" means the preferred form of the Covered Code for
making modifications to it, including all modules it contains, plus any
associated interface definition files, scripts used to control
compilation and installation of an Executable, or source code
differential comparisons against either the Original Code or another
well known, available Covered Code of the Contributor's choice. The
Source Code can be in a compressed or archival form, provided the
appropriate decompression or de-archiving software is widely available
for no charge.

36. "*Specifications*" means the specifications for the Technology and
other documentation, as designated on the Community Web Server, as may
be revised by Original Contributor from time to time.

37. "*Trademarks*" means Original Contributor's trademarks and logos,
including, but not limited to, RealNetworks, RealAudio, RealVideo,
RealOne, RealSystem, SureStream, Helix, Helix DNA and other trademarks
whether now used or adopted in the future.

38. "*Technology*" means the technology described in Attachment B, and
Upgrades.

39. "*Technology Compatibility Kit"* or *"TCK*" means the test programs,
procedures, acceptance criteria and/or other requirements, designated by
Original Contributor for use in verifying compliance of Covered Code
with the Specifications, in conjunction with the Original Code and
Upgraded Code. Original Contributor may, in its sole discretion and from
time to time, revise a TCK to correct errors and/or omissions and in
connection with Upgrades.

40. "*Upgrade(s)"* means new versions of Technology designated
exclusively by Original Contributor as an "Upgrade" and released by
Original Contributor from time to time under the terms of the License.

41. "*Upgraded Code*" means the Source Code and/or Executables for
Upgrades, possibly including Modifications made by Contributors.

42. *"User's Guide"* means the users guide for the TCK which Original
Contributor makes available to You to provide direction in how to run
the TCK and properly interpret the results, as may be revised by
Original Contributor from time to time.

43. "*You(r)*" means an individual, or a legal entity acting by and
through an individual or individuals, exercising rights either under
this License or under a future version of this License issued pursuant
to Section 4.1. For legal entities, "You(r)" includes any entity that by
majority voting interest controls, is controlled by, or is under common
control with You.

44. "*Your Products*" means any (i) hardware products You distribute
integrating the Covered Code; (ii) any software products You distribute
with the Covered Code that utilize the APIs of the Covered Code; or
(iii) any services You provide using the Covered Code.


  ATTACHMENT A

REQUIRED NOTICES


    ATTACHMENT A-1

REQUIRED IN ALL CASES

Notice to be included in header file of all Error Corrections and Shared
Modifications:

Portions Copyright 1994-2003 (c) RealNetworks, Inc. All rights reserved.

The contents of this file, and the files included with this file, are
subject to the current version of RealNetworks Community Source License
Version 1.1 (the "License"). You may not use this file except in
compliance with the License executed by both You and RealNetworks. You
may obtain a copy of the License at *
https://www.helixcommunity.org/content/rcsl.* You may also obtain a copy
of the License by contacting RealNetworks directly. Please see the
License for the rights, obligations and limitations governing use of the
contents of the file.

This file is part of the Helix DNA technology. RealNetworks, Inc., is
the developer of the Original code and owns the copyrights in the
portions it created.

This file, and the files included with this file, are distributed on an
'AS IS' basis, WITHOUT WARRANTY OF ANY KIND, EITHER EXPRESS OR IMPLIED,
AND REALNETWORKS HEREBY DISCLAIMS ALL SUCH WARRANTIES, INCLUDING WITHOUT
LIMITATION, ANY WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR
PURPOSE, QUIET ENJOYMENT OR NON-INFRINGEMENT.

Contributor(s):

_______________________________________________

Technology Compatibility Kit Test Suite(s) Location:

________________________________


    ATTACHMENT A-2

SAMPLE LICENSEE CERTIFICATION

"By clicking the `Agree' button below, You certify that You are a
Licensee in good standing under the RealNetworks Community Source
License, ("License") and that Your access, use and distribution of code
and information You may obtain at this site is subject to the License.
If You are not a Licensee under the RealNetworks Community Source
License You agree not to download, copy or use the Helix DNA technology.


    ATTACHMENT A-3

REQUIRED STUDENT NOTIFICATION

"This software and related documentation has been obtained by Your
educational institution subject to the RealNetworks Community Source
License. You have been provided access to the software and related
documentation for use only in connection with your course work and
research activities as a matriculated student of Your educational
institution. Any other use is expressly prohibited.

THIS SOFTWARE AND RELATED DOCUMENTATION CONTAINS PROPRIETARY MATERIAL OF
REALNETWORKS, INC, WHICH ARE PROTECTED BY VARIOUS INTELLECTUAL PROPERTY
RIGHTS.

You may not use this file except in compliance with the License. You may
obtain a copy of the License on the web at
https://www.helixcommunity.org/content/rcsl.

*
*


  ATTACHMENT B

Description of Technology

Helix DNA, which consists of Helix DNA Client, Helix DNA Server and
Helix DNA Producer.

Description of "Technology"

Helix DNA Technology v1.0 as described on the Community Web Server.


  ATTACHMENT C

TECHNOLOGY COMPATIBILITY KIT LICENSE

The following license is effective for the *Helix DNA* Technology
Compatibility Kit - as described on the Community Web Server. The
Technology Compatibility Kit(s) for the Technology specified in
Attachment B may be accessed at the Community Web Server.

1. TCK License.

1.1 Grants to use TCK

Subject to the terms and restrictions set forth below and the
RealNetworks Community Source License, and the Research Use license,
Original Contributor grants to You a worldwide, non-exclusive,
non-transferable license, to the extent of Original Contributor's
Intellectual Property Rights in the TCK (without the right to
sublicense), to use the TCK to develop and test Covered Code.

1.2 TCK Use Restrictions.

You are not authorized to create derivative works of the TCK or use the
TCK to test any implementation of the Specification that is not Covered
Code. You may not publish Your test results or make claims of
comparative compatibility with respect to other implementations of the
Specification. In consideration for the license grant in Section 1.1
above You agree not to develop Your own tests that are intended to
validate conformation with the Specification.

2. Test Results.

You agree to provide to Original Contributor or the third party test
facility if applicable, Your test results that demonstrate that Covered
Code is Compliant Covered Code and that Original Contributor may publish
or otherwise distribute such test results.

PLEASE READ THE TERMS OF THIS LICENSE CAREFULLY. BY CLICKING ON THE
"ACCEPT" BUTTON BELOW YOU ARE ACCEPTING AND AGREEING TO THE TERMS AND
CONDITIONS OF THIS LICENSE WITH THE ORIGINAL CONTRIBUTOR, REALNETWORKS,
INC. IF YOU ARE AGREEING TO THIS LICENSE ON BEHALF OF A COMPANY, YOU
REPRESENT THAT YOU ARE AUTHORIZED TO BIND THE COMPANY TO SUCH A LICENSE.
WHETHER YOU ARE ACTING ON YOUR OWN BEHALF, OR REPRESENTING A COMPANY,
YOU MUST BE OF MAJORITY AGE AND BE OTHERWISE COMPETENT TO ENTER INTO
CONTRACTS. IF YOU DO NOT MEET THIS CRITERIA OR YOU DO NOT AGREE TO ANY
OF THE TERMS AND CONDITIONS OF THIS LICENSE, CLICK ON THE REJECT BUTTON
TO EXIT.

*ACCEPT / REJECT
*

*
*

*To agree to the R&D/academic terms of this license, please register
<https://www.helixcommunity.org/2002/intro/why-register> on the site --
you will then be given a chance to agree to the clickwrap RCSL
<https://reguseronly.helixcommunity.org/2002/clickwrap/rcsl-clickwrap>
R&D License
<https://reguseronly.helixcommunity.org/2002/clickwrap/rcsl-clickwrap>
and gain access to the RCSL-licensed source code.  To build or deploy
commercial applications based on the RCSL, you will need to agree to the
Commercial Use license attachments
<https://www.helixcommunity.org/content/rcsl-attachments>*



~~~~~~~~~~

### Licence text K - GCC Runtime Library Exception

Text copied from: toolchain-xtensa-esp32 8.4.0+2021r2-patch5, file share/licenses/gcc/COPYING.RUNTIME

~~~~~~~~~~
GCC RUNTIME LIBRARY EXCEPTION

Version 3.1, 31 March 2009

Copyright (C) 2009 Free Software Foundation, Inc. <http://fsf.org/>

Everyone is permitted to copy and distribute verbatim copies of this
license document, but changing it is not allowed.

This GCC Runtime Library Exception ("Exception") is an additional
permission under section 7 of the GNU General Public License, version
3 ("GPLv3"). It applies to a given file (the "Runtime Library") that
bears a notice placed by the copyright holder of the file stating that
the file is governed by GPLv3 along with this Exception.

When you use GCC to compile a program, GCC may combine portions of
certain GCC header files and runtime libraries with the compiled
program. The purpose of this Exception is to allow compilation of
non-GPL (including proprietary) programs to use, in this way, the
header files and runtime libraries covered by this Exception.

0. Definitions.

A file is an "Independent Module" if it either requires the Runtime
Library for execution after a Compilation Process, or makes use of an
interface provided by the Runtime Library, but is not otherwise based
on the Runtime Library.

"GCC" means a version of the GNU Compiler Collection, with or without
modifications, governed by version 3 (or a specified later version) of
the GNU General Public License (GPL) with the option of using any
subsequent versions published by the FSF.

"GPL-compatible Software" is software whose conditions of propagation,
modification and use would permit combination with GCC in accord with
the license of GCC.

"Target Code" refers to output from any compiler for a real or virtual
target processor architecture, in executable form or suitable for
input to an assembler, loader, linker and/or execution
phase. Notwithstanding that, Target Code does not include data in any
format that is used as a compiler intermediate representation, or used
for producing a compiler intermediate representation.

The "Compilation Process" transforms code entirely represented in
non-intermediate languages designed for human-written code, and/or in
Java Virtual Machine byte code, into Target Code. Thus, for example,
use of source code generators and preprocessors need not be considered
part of the Compilation Process, since the Compilation Process can be
understood as starting with the output of the generators or
preprocessors.

A Compilation Process is "Eligible" if it is done using GCC, alone or
with other GPL-compatible software, or if it is done without using any
work based on GCC. For example, using non-GPL-compatible Software to
optimize any GCC intermediate representations would not qualify as an
Eligible Compilation Process.

1. Grant of Additional Permission.

You have permission to propagate a work of Target Code formed by
combining the Runtime Library with Independent Modules, even if such
propagation would otherwise violate the terms of GPLv3, provided that
all Target Code was generated by Eligible Compilation Processes. You
may then convey such a combination under terms of your choice,
consistent with the licensing of the Independent Modules.

2. No Weakening of GCC Copyleft.

The availability of this Exception does not imply any general
presumption that third-party software is unaffected by the copyleft
requirements of the license of GCC.

~~~~~~~~~~

### Licence text L - Newlib licence notices

Text copied from: toolchain-xtensa-esp32 8.4.0+2021r2-patch5, file share/licenses/newlib/COPYING.NEWLIB

~~~~~~~~~~
The newlib subdirectory is a collection of software from several sources.

Each file may have its own copyright/license that is embedded in the source 
file.  Unless otherwise noted in the body of the source file(s), the following copyright
notices will apply to the contents of the newlib subdirectory:

(1) Red Hat Incorporated

Copyright (c) 1994-2009  Red Hat, Inc. All rights reserved.

This copyrighted material is made available to anyone wishing to use,
modify, copy, or redistribute it subject to the terms and conditions
of the BSD License.   This program is distributed in the hope that 
it will be useful, but WITHOUT ANY WARRANTY expressed or implied, 
including the implied warranties of MERCHANTABILITY or FITNESS FOR 
A PARTICULAR PURPOSE.  A copy of this license is available at 
http://www.opensource.org/licenses. Any Red Hat trademarks that are
incorporated in the source code or documentation are not subject to
the BSD License and may only be used or replicated with the express
permission of Red Hat, Inc.

(2) University of California, Berkeley

Copyright (c) 1981-2000 The Regents of the University of California.
All rights reserved.

Redistribution and use in source and binary forms, with or without modification,
are permitted provided that the following conditions are met:

    * Redistributions of source code must retain the above copyright notice, 
      this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright notice,
      this list of conditions and the following disclaimer in the documentation
      and/or other materials provided with the distribution.
    * Neither the name of the University nor the names of its contributors 
      may be used to endorse or promote products derived from this software 
      without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" 
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED 
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. 
IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, 
INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT 
NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR 
PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, 
WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) 
ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY
OF SUCH DAMAGE.

(3) David M. Gay (AT&T 1991, Lucent 1998)

The author of this software is David M. Gay.

Copyright (c) 1991 by AT&T.

Permission to use, copy, modify, and distribute this software for any
purpose without fee is hereby granted, provided that this entire notice
is included in all copies of any software which is or includes a copy
or modification of this software and in all copies of the supporting
documentation for such software.

THIS SOFTWARE IS BEING PROVIDED "AS IS", WITHOUT ANY EXPRESS OR IMPLIED
WARRANTY.  IN PARTICULAR, NEITHER THE AUTHOR NOR AT&T MAKES ANY
REPRESENTATION OR WARRANTY OF ANY KIND CONCERNING THE MERCHANTABILITY
OF THIS SOFTWARE OR ITS FITNESS FOR ANY PARTICULAR PURPOSE.

-------------------------------------------------------------------

The author of this software is David M. Gay.

Copyright (C) 1998-2001 by Lucent Technologies
All Rights Reserved

Permission to use, copy, modify, and distribute this software and
its documentation for any purpose and without fee is hereby
granted, provided that the above copyright notice appear in all
copies and that both that the copyright notice and this
permission notice and warranty disclaimer appear in supporting
documentation, and that the name of Lucent or any of its entities
not be used in advertising or publicity pertaining to
distribution of the software without specific, written prior
permission.

LUCENT DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE,
INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS.
IN NO EVENT SHALL LUCENT OR ANY OF ITS ENTITIES BE LIABLE FOR ANY
SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER
IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION,
ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF
THIS SOFTWARE.


(4) Advanced Micro Devices

Copyright 1989, 1990 Advanced Micro Devices, Inc.

This software is the property of Advanced Micro Devices, Inc  (AMD)  which
specifically  grants the user the right to modify, use and distribute this
software provided this notice is not removed or altered.  All other rights
are reserved by AMD.

AMD MAKES NO WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, WITH REGARD TO THIS
SOFTWARE.  IN NO EVENT SHALL AMD BE LIABLE FOR INCIDENTAL OR CONSEQUENTIAL
DAMAGES IN CONNECTION WITH OR ARISING FROM THE FURNISHING, PERFORMANCE, OR
USE OF THIS SOFTWARE.

So that all may benefit from your experience, please report  any  problems
or  suggestions about this software to the 29K Technical Support Center at
800-29-29-AMD (800-292-9263) in the USA, or 0800-89-1131  in  the  UK,  or
0031-11-1129 in Japan, toll free.  The direct dial number is 512-462-4118.

Advanced Micro Devices, Inc.
29K Support Products
Mail Stop 573
5900 E. Ben White Blvd.
Austin, TX 78741
800-292-9263

(5) 

(6)

(7) Sun Microsystems

Copyright (C) 1993 by Sun Microsystems, Inc. All rights reserved.

Developed at SunPro, a Sun Microsystems, Inc. business.
Permission to use, copy, modify, and distribute this
software is freely granted, provided that this notice is preserved.

(8) Hewlett Packard

(c) Copyright 1986 HEWLETT-PACKARD COMPANY

To anyone who acknowledges that this file is provided "AS IS"
without any express or implied warranty:
    permission to use, copy, modify, and distribute this file
for any purpose is hereby granted without fee, provided that
the above copyright notice and this notice appears in all
copies, and that the name of Hewlett-Packard Company not be
used in advertising or publicity pertaining to distribution
of the software without specific, written prior permission.
Hewlett-Packard Company makes no representations about the
suitability of this software for any purpose.

(9) Hans-Peter Nilsson

Copyright (C) 2001 Hans-Peter Nilsson

Permission to use, copy, modify, and distribute this software is
freely granted, provided that the above copyright notice, this notice
and the following disclaimer are preserved with no changes.

THIS SOFTWARE IS PROVIDED ``AS IS'' AND WITHOUT ANY EXPRESS OR
IMPLIED WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
PURPOSE.

(10) Stephane Carrez (m68hc11-elf/m68hc12-elf targets only)

Copyright (C) 1999, 2000, 2001, 2002 Stephane Carrez (stcarrez@nerim.fr)

The authors hereby grant permission to use, copy, modify, distribute,
and license this software and its documentation for any purpose, provided
that existing copyright notices are retained in all copies and that this
notice is included verbatim in any distributions. No written agreement,
license, or royalty fee is required for any of the authorized uses.
Modifications to this software may be copyrighted by their authors
and need not follow the licensing terms described here, provided that
the new terms are clearly indicated on the first page of each file where
they apply.

(11) Christopher G. Demetriou

Copyright (c) 2001 Christopher G. Demetriou
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions
are met:
1. Redistributions of source code must retain the above copyright
   notice, this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright
   notice, this list of conditions and the following disclaimer in the
   documentation and/or other materials provided with the distribution.
3. The name of the author may not be used to endorse or promote products
   derived from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

(12) SuperH, Inc.

Copyright 2002 SuperH, Inc. All rights reserved

This software is the property of SuperH, Inc (SuperH) which specifically
grants the user the right to modify, use and distribute this software
provided this notice is not removed or altered.  All other rights are
reserved by SuperH.

SUPERH MAKES NO WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, WITH REGARD TO
THIS SOFTWARE.  IN NO EVENT SHALL SUPERH BE LIABLE FOR INDIRECT, SPECIAL,
INCIDENTAL OR CONSEQUENTIAL DAMAGES IN CONNECTION WITH OR ARISING FROM
THE FURNISHING, PERFORMANCE, OR USE OF THIS SOFTWARE.

So that all may benefit from your experience, please report any problems
or suggestions about this software to the SuperH Support Center via
e-mail at softwaresupport@superh.com .

SuperH, Inc.
405 River Oaks Parkway
San Jose
CA 95134
USA

(13) Royal Institute of Technology

Copyright (c) 1999 Kungliga Tekniska Högskolan
(Royal Institute of Technology, Stockholm, Sweden).
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions
are met:

1. Redistributions of source code must retain the above copyright
   notice, this list of conditions and the following disclaimer.

2. Redistributions in binary form must reproduce the above copyright
   notice, this list of conditions and the following disclaimer in the
   documentation and/or other materials provided with the distribution.

3. Neither the name of KTH nor the names of its contributors may be
   used to endorse or promote products derived from this software without
   specific prior written permission.

THIS SOFTWARE IS PROVIDED BY KTH AND ITS CONTRIBUTORS ``AS IS'' AND ANY
EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL KTH OR ITS CONTRIBUTORS BE
LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

(14) Alexey Zelkin

Copyright (c) 2000, 2001 Alexey Zelkin <phantom@FreeBSD.org>
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions
are met:
1. Redistributions of source code must retain the above copyright
   notice, this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright
   notice, this list of conditions and the following disclaimer in the
   documentation and/or other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
SUCH DAMAGE.

(15) Andrey A. Chernov

Copyright (C) 1997 by Andrey A. Chernov, Moscow, Russia.
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions
are met:
1. Redistributions of source code must retain the above copyright
   notice, this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright
   notice, this list of conditions and the following disclaimer in the
   documentation and/or other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
SUCH DAMAGE.

(16) FreeBSD

Copyright (c) 1997-2002 FreeBSD Project.
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions
are met:
1. Redistributions of source code must retain the above copyright
   notice, this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright
   notice, this list of conditions and the following disclaimer in the
   documentation and/or other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
SUCH DAMAGE.

(17) S. L. Moshier

Author:  S. L. Moshier.

Copyright (c) 1984,2000 S.L. Moshier

Permission to use, copy, modify, and distribute this software for any
purpose without fee is hereby granted, provided that this entire notice
is included in all copies of any software which is or includes a copy
or modification of this software and in all copies of the supporting
documentation for such software.

THIS SOFTWARE IS BEING PROVIDED "AS IS", WITHOUT ANY EXPRESS OR IMPLIED
WARRANTY.  IN PARTICULAR,  THE AUTHOR MAKES NO REPRESENTATION
OR WARRANTY OF ANY KIND CONCERNING THE MERCHANTABILITY OF THIS
SOFTWARE OR ITS FITNESS FOR ANY PARTICULAR PURPOSE.

(18) Citrus Project

Copyright (c)1999 Citrus Project,
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions
are met:
1. Redistributions of source code must retain the above copyright
   notice, this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright
   notice, this list of conditions and the following disclaimer in the
   documentation and/or other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
SUCH DAMAGE.

(19) Todd C. Miller

Copyright (c) 1998 Todd C. Miller <Todd.Miller@courtesan.com>
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions
are met:
1. Redistributions of source code must retain the above copyright
   notice, this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright
   notice, this list of conditions and the following disclaimer in the
   documentation and/or other materials provided with the distribution.
3. The name of the author may not be used to endorse or promote products
   derived from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES,
INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY
AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL
THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

(20) DJ Delorie (i386 / arm)
Copyright (C) 1991 DJ Delorie
All rights reserved.

Redistribution, modification, and use in source and binary forms is permitted
provided that the above copyright notice and following paragraph are
duplicated in all such forms.

This file is distributed WITHOUT ANY WARRANTY; without even the implied
warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

(21) Free Software Foundation LGPL License (*-linux* targets only)

   Copyright (C) 1990-1999, 2000, 2001    Free Software Foundation, Inc.
   This file is part of the GNU C Library.
   Contributed by Mark Kettenis <kettenis@phys.uva.nl>, 1997.

   The GNU C Library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public
   License as published by the Free Software Foundation; either
   version 2.1 of the License, or (at your option) any later version.

   The GNU C Library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public
   License along with the GNU C Library; if not, write to the Free
   Software Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
   02110-1301 USA.

(22) Xavier Leroy LGPL License (i[3456]86-*-linux* targets only)

Copyright (C) 1996 Xavier Leroy (Xavier.Leroy@inria.fr)

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU Library General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU Library General Public License for more details.

(23) Intel (i960)

Copyright (c) 1993 Intel Corporation

Intel hereby grants you permission to copy, modify, and distribute this
software and its documentation.  Intel grants this permission provided
that the above copyright notice appears in all copies and that both the
copyright notice and this permission notice appear in supporting
documentation.  In addition, Intel grants this permission provided that
you prominently mark as "not part of the original" any modifications
made to this software or documentation, and that the name of Intel
Corporation not be used in advertising or publicity pertaining to
distribution of the software or the documentation without specific,
written prior permission.

Intel Corporation provides this AS IS, WITHOUT ANY WARRANTY, EXPRESS OR
IMPLIED, INCLUDING, WITHOUT LIMITATION, ANY WARRANTY OF MERCHANTABILITY
OR FITNESS FOR A PARTICULAR PURPOSE.  Intel makes no guarantee or
representations regarding the use of, or the results of the use of,
the software and documentation in terms of correctness, accuracy,
reliability, currentness, or otherwise; and you rely on the software,
documentation and results solely at your own risk.

IN NO EVENT SHALL INTEL BE LIABLE FOR ANY LOSS OF USE, LOSS OF BUSINESS,
LOSS OF PROFITS, INDIRECT, INCIDENTAL, SPECIAL OR CONSEQUENTIAL DAMAGES
OF ANY KIND.  IN NO EVENT SHALL INTEL'S TOTAL LIABILITY EXCEED THE SUM
PAID TO INTEL FOR THE PRODUCT LICENSED HEREUNDER.

(24) Hewlett-Packard  (hppa targets only)

(c) Copyright 1986 HEWLETT-PACKARD COMPANY

To anyone who acknowledges that this file is provided "AS IS"
without any express or implied warranty:
    permission to use, copy, modify, and distribute this file
for any purpose is hereby granted without fee, provided that
the above copyright notice and this notice appears in all
copies, and that the name of Hewlett-Packard Company not be
used in advertising or publicity pertaining to distribution
of the software without specific, written prior permission.
Hewlett-Packard Company makes no representations about the
suitability of this software for any purpose.

(25) Henry Spencer (only *-linux targets)

Copyright 1992, 1993, 1994 Henry Spencer.  All rights reserved.
This software is not subject to any license of the American Telephone
and Telegraph Company or of the Regents of the University of California.

Permission is granted to anyone to use this software for any purpose on
any computer system, and to alter it and redistribute it, subject
to the following restrictions:

1. The author is not responsible for the consequences of use of this
   software, no matter how awful, even if they arise from flaws in it.

2. The origin of this software must not be misrepresented, either by
   explicit claim or by omission.  Since few users ever read sources,
   credits must appear in the documentation.

3. Altered versions must be plainly marked as such, and must not be
   misrepresented as being the original software.  Since few users
   ever read sources, credits must appear in the documentation.

4. This notice may not be removed or altered.

(26) Mike Barcroft

Copyright (c) 2001 Mike Barcroft <mike@FreeBSD.org>
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions
are met:
1. Redistributions of source code must retain the above copyright
   notice, this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright
   notice, this list of conditions and the following disclaimer in the
   documentation and/or other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
SUCH DAMAGE.

(27) Konstantin Chuguev (--enable-newlib-iconv)

Copyright (c) 1999, 2000
   Konstantin Chuguev.  All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions
are met:
1. Redistributions of source code must retain the above copyright
   notice, this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright
   notice, this list of conditions and the following disclaimer in the
   documentation and/or other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
SUCH DAMAGE.

   iconv (Charset Conversion Library) v2.0

(28) Artem Bityuckiy (--enable-newlib-iconv)

Copyright (c) 2003, Artem B. Bityuckiy, SoftMine Corporation.
Rights transferred to Franklin Electronic Publishers.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions
are met:
1. Redistributions of source code must retain the above copyright
   notice, this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright
   notice, this list of conditions and the following disclaimer in the
   documentation and/or other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
SUCH DAMAGE.

(29) IBM, Sony, Toshiba (only spu-* targets)

  (C) Copyright 2001,2006,
  International Business Machines Corporation,
  Sony Computer Entertainment, Incorporated,
  Toshiba Corporation,

  All rights reserved.

  Redistribution and use in source and binary forms, with or without
  modification, are permitted provided that the following conditions are met:

    * Redistributions of source code must retain the above copyright notice,
      this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright
      notice, this list of conditions and the following disclaimer in the
      documentation and/or other materials provided with the distribution.
    * Neither the names of the copyright holders nor the names of their
      contributors may be used to endorse or promote products derived from this
      software without specific prior written permission.

  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
  AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
  IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
  ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
  LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
  CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
  SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
  INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
  CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
  ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
  POSSIBILITY OF SUCH DAMAGE.

(30) - Alex Tatmanjants (targets using libc/posix)

  Copyright (c) 1995 Alex Tatmanjants <alex@elvisti.kiev.ua>
 		at Electronni Visti IA, Kiev, Ukraine.
 			All rights reserved.
 
  Redistribution and use in source and binary forms, with or without
  modification, are permitted provided that the following conditions
  are met:
  1. Redistributions of source code must retain the above copyright
     notice, this list of conditions and the following disclaimer.
  2. Redistributions in binary form must reproduce the above copyright
     notice, this list of conditions and the following disclaimer in the
     documentation and/or other materials provided with the distribution.
 
  THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND
  ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
  IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
  ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR BE LIABLE
  FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
  DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
  OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
  HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
  LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
  OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
  SUCH DAMAGE.

(31) - M. Warner Losh (targets using libc/posix)

  Copyright (c) 1998, M. Warner Losh <imp@freebsd.org>
  All rights reserved.
 
  Redistribution and use in source and binary forms, with or without
  modification, are permitted provided that the following conditions
  are met:
  1. Redistributions of source code must retain the above copyright
     notice, this list of conditions and the following disclaimer.
  2. Redistributions in binary form must reproduce the above copyright
     notice, this list of conditions and the following disclaimer in the
     documentation and/or other materials provided with the distribution.
 
  THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
  ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
  IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
  ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
  FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
  DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
  OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
  HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
  LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
  OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
  SUCH DAMAGE.

(32) - Andrey A. Chernov (targets using libc/posix)

  Copyright (C) 1996 by Andrey A. Chernov, Moscow, Russia.
  All rights reserved.
 
  Redistribution and use in source and binary forms, with or without
  modification, are permitted provided that the following conditions
  are met:
  1. Redistributions of source code must retain the above copyright
     notice, this list of conditions and the following disclaimer.
  2. Redistributions in binary form must reproduce the above copyright
     notice, this list of conditions and the following disclaimer in the
     documentation and/or other materials provided with the distribution.
 
  THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND
  ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
  IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
  ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
  FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
  DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
  OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
  HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
  LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
  OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
  SUCH DAMAGE.

(33) - Daniel Eischen (targets using libc/posix)

  Copyright (c) 2001 Daniel Eischen <deischen@FreeBSD.org>.
  All rights reserved.
 
  Redistribution and use in source and binary forms, with or without
  modification, are permitted provided that the following conditions
  are met:
  1. Redistributions of source code must retain the above copyright
     notice, this list of conditions and the following disclaimer.
  2. Redistributions in binary form must reproduce the above copyright
     notice, this list of conditions and the following disclaimer in the
     documentation and/or other materials provided with the distribution.
 
  THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
  ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
  IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
  ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
  FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
  DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
  OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
  HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
  LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
  OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
  SUCH DAMAGE.


(34) - Jon Beniston (only lm32-* targets)

 Contributed by Jon Beniston <jon@beniston.com>

 Redistribution and use in source and binary forms, with or without
 modification, are permitted provided that the following conditions
 are met:
 1. Redistributions of source code must retain the above copyright
 notice, this list of conditions and the following disclaimer.
 2. Redistributions in binary form must reproduce the above copyright
 notice, this list of conditions and the following disclaimer in the
 documentation and/or other materials provided with the distribution.

 THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 SUCH DAMAGE.


(35) - Arm Ltd

 Copyright (c) 2009-2018 Arm Ltd
 All rights reserved.
 
 Redistribution and use in source and binary forms, with or without
 modification, are permitted provided that the following conditions
 are met:
 1. Redistributions of source code must retain the above copyright
    notice, this list of conditions and the following disclaimer.
 2. Redistributions in binary form must reproduce the above copyright
    notice, this list of conditions and the following disclaimer in the
    documentation and/or other materials provided with the distribution.
 3. The name of the company may not be used to endorse or promote
    products derived from this software without specific prior written
    permission.

 THIS SOFTWARE IS PROVIDED BY ARM LTD ``AS IS'' AND ANY EXPRESS OR IMPLIED
 WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 IN NO EVENT SHALL ARM LTD BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED
 TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

(36) - Xilinx, Inc. (microblaze-* and powerpc-* targets)

Copyright (c) 2004, 2009 Xilinx, Inc.  All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are
met:

1.  Redistributions source code must retain the above copyright notice,
this list of conditions and the following disclaimer.

2.  Redistributions in binary form must reproduce the above copyright
notice, this list of conditions and the following disclaimer in the
documentation and/or other materials provided with the distribution.

3.  Neither the name of Xilinx nor the names of its contributors may be
used to endorse or promote products derived from this software without
specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDER AND CONTRIBUTORS "AS
IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED
TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.


(37) Texas Instruments Incorporated (tic6x-*, *-tirtos targets)

Copyright (c) 1996-2010,2014 Texas Instruments Incorporated
http://www.ti.com/

 Redistribution and  use in source  and binary forms, with  or without
 modification,  are permitted provided  that the  following conditions
 are met:

    Redistributions  of source  code must  retain the  above copyright
    notice, this list of conditions and the following disclaimer.

    Redistributions in binary form  must reproduce the above copyright
    notice, this  list of conditions  and the following  disclaimer in
    the  documentation  and/or   other  materials  provided  with  the
    distribution.

    Neither the  name of Texas Instruments Incorporated  nor the names
    of its  contributors may  be used to  endorse or  promote products
    derived  from   this  software  without   specific  prior  written
    permission.

 THIS SOFTWARE  IS PROVIDED BY THE COPYRIGHT  HOLDERS AND CONTRIBUTORS
 "AS IS"  AND ANY  EXPRESS OR IMPLIED  WARRANTIES, INCLUDING,  BUT NOT
 LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 A PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT
 OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 SPECIAL,  EXEMPLARY,  OR CONSEQUENTIAL  DAMAGES  (INCLUDING, BUT  NOT
 LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 THEORY OF  LIABILITY, WHETHER IN CONTRACT, STRICT  LIABILITY, OR TORT
 (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

(38) National Semiconductor (cr16-* and crx-* targets)

Copyright (c) 2004 National Semiconductor Corporation

The authors hereby grant permission to use, copy, modify, distribute,
and license this software and its documentation for any purpose, provided
that existing copyright notices are retained in all copies and that this
notice is included verbatim in any distributions. No written agreement,
license, or royalty fee is required for any of the authorized uses.
Modifications to this software may be copyrighted by their authors
and need not follow the licensing terms described here, provided that
the new terms are clearly indicated on the first page of each file where
they apply. 

(39) - Adapteva, Inc. (epiphany-* targets)

Copyright (c) 2011, Adapteva, Inc.
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:
 * Redistributions of source code must retain the above copyright notice, this
   list of conditions and the following disclaimer.
 * Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.
 * Neither the name of Adapteva nor the names of its contributors may be used
   to endorse or promote products derived from this software without specific
   prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

(40) - Altera Corportion (nios2-* targets)

Copyright (c) 2003 Altera Corporation
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions
are met:

   o Redistributions of source code must retain the above copyright
     notice, this list of conditions and the following disclaimer. 
   o Redistributions in binary form must reproduce the above copyright
     notice, this list of conditions and the following disclaimer in the 
     documentation and/or other materials provided with the distribution. 
   o Neither the name of Altera Corporation nor the names of its 
     contributors may be used to endorse or promote products derived from
     this software without specific prior written permission. 
 
THIS SOFTWARE IS PROVIDED BY ALTERA CORPORATION, THE COPYRIGHT HOLDER,
AND ITS CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES,
INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY
AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL
THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS
OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR
TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE
USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.  

(41) Ed Schouten - Free BSD

Copyright (c) 2008 Ed Schouten <ed@FreeBSD.org>
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions
are met:
1. Redistributions of source code must retain the above copyright
   notice, this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright
   notice, this list of conditions and the following disclaimer in the
   documentation and/or other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
SUCH DAMAGE.

(42) - Rolls-Royce Controls and Data Services Limited (visium-* targets)

Copyright (c) 2015 Rolls-Royce Controls and Data Services Limited.
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

  * Redistributions of source code must retain the above copyright notice,
    this list of conditions and the following disclaimer.
  * Redistributions in binary form must reproduce the above copyright
    notice, this list of conditions and the following disclaimer in the
    documentation and/or other materials provided with the distribution.
  * Neither the name of Rolls-Royce Controls and Data Services Limited nor
    the names of its contributors may be used to endorse or promote products
    derived from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

(43) - FTDI (ft32-* targets)

Copyright (C) 2014 FTDI (support@ftdichip.com)

The authors hereby grant permission to use, copy, modify, distribute,
and license this software and its documentation for any purpose, provided
that existing copyright notices are retained in all copies and that this
notice is included verbatim in any distributions. No written agreement,
license, or royalty fee is required for any of the authorized uses.
Modifications to this software may be copyrighted by their authors
and need not follow the licensing terms described here, provided that
the new terms are clearly indicated on the first page of each file where
they apply.

(44) - Synopsys Inc (arc-* targets)

Copyright (c) 2015, Synopsys, Inc. All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

1) Redistributions of source code must retain the above copyright notice,
this list of conditions and the following disclaimer.

2) Redistributions in binary form must reproduce the above copyright notice,
this list of conditions and the following disclaimer in the documentation
and/or other materials provided with the distribution.

3) Neither the name of the Synopsys, Inc., nor the names of its contributors
may be used to endorse or promote products derived from this software
without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
POSSIBILITY OF SUCH DAMAGE.

(45) Phoenix Systems - Phoenix-RTOS targets

Copyright (c) 2016 Phoenix Systems
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions
are met:
1. Redistributions of source code must retain the above copyright
   notice, this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright
   notice, this list of conditions and the following disclaimer in the
   documentation and/or other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
SUCH DAMAGE.

(46) embedded brains - RTEMS targets

Copyright (c) 2017 embedded brains GmbH
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions
are met:
1. Redistributions of source code must retain the above copyright
   notice, this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright
   notice, this list of conditions and the following disclaimer in the
   documentation and/or other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
SUCH DAMAGE.

(47) John Baldwin - RTEMS targets

Copyright (c) 2015 John Baldwin <jhb@FreeBSD.org>.
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions
are met:
1. Redistributions of source code must retain the above copyright
   notice, this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright
   notice, this list of conditions and the following disclaimer in the
   documentation and/or other materials provided with the distribution.
3. Neither the name of the author nor the names of its contributors
   may be used to endorse or promote products derived from this software
   without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
SUCH DAMAGE.


(48) Jeffrey Roberson - RTEMS targets

Copyright (c) 2008, Jeffrey Roberson <jeff@freebsd.org>
All rights reserved.

Copyright (c) 2008 Nokia Corporation
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions
are met:
1. Redistributions of source code must retain the above copyright
   notice unmodified, this list of conditions, and the following
   disclaimer.
2. Redistributions in binary form must reproduce the above copyright
   notice, this list of conditions and the following disclaimer in the
   documentation and/or other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

(49) - SiFive Inc. (riscv-* targets)

Copyright (c) 2017  SiFive Inc. All rights reserved.

This copyrighted material is made available to anyone wishing to use,
modify, copy, or redistribute it subject to the terms and conditions
of the FreeBSD License.   This program is distributed in the hope that
it will be useful, but WITHOUT ANY WARRANTY expressed or implied,
including the implied warranties of MERCHANTABILITY or FITNESS FOR
A PARTICULAR PURPOSE.  A copy of this license is available at
http://www.opensource.org/licenses.

(50) Michael R. Neilly (riscv-* targets)

(c) Copyright 2017 Michael R. Neilly
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions
are met:

* Redistributions of source code must retain the above copyright
notice, this list of conditions and the following disclaimer.

* Redistributions in binary form must reproduce the above copyright
notice, this list of conditions and the following disclaimer in the
documentation and/or other materials provided with the distribution.

* Neither the names of the copyright holders nor the names of their
contributors may be used to endorse or promote products derived from
this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
"AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
POSSIBILITY OF SUCH DAMAGE.

(51) Mentor Graphics (amdgcn-* targets)

Copyright (c) 2014-2017 Mentor Graphics.

The authors hereby grant permission to use, copy, modify, distribute,
and license this software and its documentation for any purpose, provided
that existing copyright notices are retained in all copies and that this
notice is included verbatim in any distributions. No written agreement,
license, or royalty fee is required for any of the authorized uses.
Modifications to this software may be copyrighted by their authors
and need not follow the licensing terms described here, provided that
the new terms are clearly indicated on the first page of each file where
they apply.

(52) BSD-2-Clause-FreeBSD (pru-* targets) 

SPDX-License-Identifier: BSD-2-Clause-FreeBSD

Copyright (c) 2018-2019 Dimitar Dimitrov <dimitar@dinux.eu>
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions
are met:
1. Redistributions of source code must retain the above copyright
   notice, this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright
   notice, this list of conditions and the following disclaimer in the
   documentation and/or other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

~~~~~~~~~~

