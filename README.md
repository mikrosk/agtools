# Atari Game Tools #

         .__________________________________________________.
         ||////////////////////////////////////////////////||
         ||////////////////////////////////////////////////||
         ||////////////////////////////////////////////////||
         ||////////////////////////////////////////////////||    ___.
         ||////////////////////////////////////////////////||   /    \
         !__________________________________________________!  |      |
         |   __ __ __ __ __ __ __ __ __ __  /|\ ATARI 1040ST|  |      |
         |__/_//_//_//_//_//_//_//_//_//_/____________--____|  |  .---|---.
         | ______________________________________________   |  |  |   |   |
         | [][][][][][][][][][][][][][][__] [_][_] [][][][] |  |  |---'---|
         | [_][][][][][][][][][][][][]| |[] [][][] [][][][] |  |  |       |
         | [__][][][][][][][][][][][][__|[] [][][] [][][][] |  |  |       |
         | [_][][][][][][][][][][][][_]            [][][]|| |  |  |  /|\  |
         |    [_][________________][_]             [__][]LI |  |   \_____/
         |__________________________________________________|  ;
                                                          \___/



### What is this repository for? ###

* Tools and code for rapid prototyping of Atari STE games
* Game framework samples

### How do I get set up? ###

* To set up a build environment you'll need Atari MiNT-GCC 4.6.4 (or one of the other GCC compilers now available) and a few other prerequisites. See [this page](https://bitbucket.org/d_m_l/agtools/wiki/Host%20Development%20Environments) for details on different host platforms.
* The main AGT tools are already provided in the project /bin directory. These should work on Windows and MacOSX with a [suitably configured environment](https://bitbucket.org/d_m_l/agtools/wiki/Host%20Development%20Environments)
* The project is Make-driven which requires a specific environment on Windows: Cygwin, MinGW or WSL (typically has been Cygwin since a prebuilt GCC 4.6.4 package is available for it)
* On all systems, the GCC compiler tools should be configured on the default $PATH to work - so test that first!
* An emulator will help (Hatari, STEEM, SainT) but nothing beats real hardware!
* *   [Hatari - Windows nightly builds](http://antarctica.no/~hatari/)
* *   [STEEM Engine](http://steem.atari.st/)
* *   STEEM SSE / STEEM Boiler (debugger)


### Reference ###

* the AGT wiki can be found [here](https://bitbucket.org/d_m_l/agtools/wiki/Home)
* the RMAC assembler manual can be found [here](http://rmac.is-slick.com/)
