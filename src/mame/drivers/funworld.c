/**********************************************************************************


    FUNWORLD.


    Original preliminary driver:    Curt Coder, Peter Trauner.
    Rewrite and aditional work:     Roberto Fresca.


    Games running in this hardware:

    * Jolly Card (Austria),                     TAB-Austria,        1985.
    * Jolly Card (Austria, encrypted),          TAB-Austria,        1985.
    * Jolly Card (Croatia),                     Soft Design,        1993.
    * Jolly Card (Italia, encrypted),           bootleg,            199?.
    * Jolly Card (Austria, Fun World, bootleg), Inter Games,        1995.
    * Big Deal (Hungary, set 1),                Fun World,          1990.
    * Big Deal (Hungary, set 2),                Fun World,          1990.
    * Jolly Card (Austria, Fun World),          Fun World,          1990.
    * Cuore 1 (Italia),                         C.M.C.,             1996.
    * Elephant Family (Italia, new),            C.M.C.,             1997.
    * Elephant Family (Italia, old),            C.M.C.,             1996.
    * Pool 10 (Italia, set 1),                  C.M.C.,             1996.
    * Pool 10 (Italia, set 2),                  C.M.C.,             1996.
    * Tortuga Family (Italia),                  C.M.C.,             1997.
    * Royal Card (Austria, set 1),              TAB-Austria,        1991.
    * Royal Card (Austria, set 2),              TAB-Austria,        1991.
    * Royal Card (Slovakia, encrypted),         Evona Electronic,   1991.
    * Magic Card II (Bulgaria, bootleg),        Impera,             1996.
    * Joker Card (Ver.A267BC, encrypted),       Vesely Svet,        1993.
    * Mongolfier New (Italia),                  bootleg,            199?.
    * Soccer New (Italia),                      bootleg,            199?.


***********************************************************************************


    The hardware is generally composed by:

    CPU:    1x 65SC02 or 65C02 at 2MHz.
    Sound:  1x AY3-8910 or YM2149F (AY8910 compatible) at 2MHz.
    I/O:    2x 6821 (PIA)
    Video:  1x 6845 (CRTC)
    RAM:    1x 6116
    NVRAM:  1x 6264
    ROMs:   3x 27256 (or 27512 in some cases)
    PROMs:  1x 82S147 (or similar. 512 bytes)
    PLDs:   1 to 4 PALs, GALs or PEELs
    Clock:  1x Crystal: 16MHz.


    All current games are running from a slightly modified to heavily hacked hardware.
    Color palettes are normally stored in format RRRBBBGG inside a bipolar color PROM.

    - bits -
    7654 3210
    ---- -xxx   Red component.
    --xx x---   Blue component.
    xx-- ----   Green component.


    The hardware was designed to manage 4096 tiles with a size of 8x4 pixels each.
    Also support 4bpp graphics and the palette limitation is 8 bits for color codes (256 x 16colors).
    It means the hardware was designed for more elaborated graphics than Jolly Card games...
    Color PROMs from current games are 512 bytes lenght, but they only use the first 256 bytes.

    Normal hardware capabilities:

    - bits -
    7654 3210
    xxxx xx--   tiles color (game tiles)    ;codes 0x00-0xdc
    xxx- x-xx   tiles color (title).        :codes 0xe9-0xeb
    xxxx -xxx   tiles color (background).   ;codes 0xf1-0xf7


    About protection, there are several degrees of protection found in the sets:

    - There are writes to unknown offsets (out of the normal memory range), and some
      checks later to see if the data is still there.

    - There are checks for code in unused RAM and therefore jumps to offsets where there
      are not pieces of code in RAM or just RAM is inexistent.
      I think this is to avoid a "ROM swap" that allow the software to run in other game boards.

    - There are "masked" unused inputs.
      The software is polling the unused input status and expect a special value to boot the game.

    - There are parts of code that are very complex and twisted with fake jumps to inexistent code,
      or pretending to initialize fake devices.

    - Encryption.

            A) Encrypted CPU. At least two Fun World boards have custom encrypted CPUs:

                - Joker Card from Vesely Svet use a custom unknown CPU and use encrypted prg roms.
                - Royal Card (Slovakia, encrypted) from Evona Electronic seems to use a block
                  with CPU + extras (ICs, TTL, etc) to manage the encryption.

            B) General encryption. Managed through hardware:

                - Jolly Card (Italia, encrypted GFXs) use substitution for each byte nibble. See DRIVER_INIT for the algorythm.
                - Jolly Card (Austria, encrypted) use simple XOR with a fixed value.

    - Microcontroller. Some games are using an extra microcontroller mainly for protection.



    GENERAL NOTES:

    - It takes 46 seconds for the bigdeal/jolycdat games to boot up
      after the initial screen is displayed!!!

    - The default DIP switch settings must be used when first booting up
      the games to allow them to complete the NVRAM initialization.

    - Almost all games: Start game, press and hold Service1 & Service2, press
      reset (F3), release Service1/2 and press reset (F3) again.
      Now the NVRAM has been initialized.

    - Royalcdb needs a hard reset after NVRAM initialization.

    - For games that allow remote credits, after NVRAM init change the payout
      DIP switch from "Hopper" to "Manual Payout".



    NOTES BY GAME/SET:

    - Pool 10
    - Cuore 1
    - Elephant Family
    - Tortuga Family

    In Italy many people became addicted to videopokers. They put so much money on them,
    and they had to sell the house. Also some engineers modified videopokers to do less
    wins and so on... Because of this the government did some laws in order to regulate
    videopokers wins. Starting from around 1996/1997 there were subsequent laws because
    engineers always found a way to elude them.

    Today all the videopokers need to be connected via AAMS net (a government society de-
    dicated to games) which check if the videopoker is regular. Nowadays it's difficult
    to trick and the videopoker has to give 75% of wins. This has made videopoker market
    to collapse and infact there aren't many videopokers left.

    Also because the laws changed very often and old videopokers became illegal was a
    very bad thing for bar owners because they couldn't earn enough money.

    Pool 10 (now found!), apparently was the "father" of other italian gambling games.
    As soon as it became illegal, was converted to Cuore 1, Elephant Family, Tortuga Family
    and maybe other games. So you can see that engineers always found a simple way to elude
    the law.

    There is another set of Cuore 1. I didn't include it because the only difference with
    the supported set is the program rom that is double sized, having identical halves.



    - Jolly Card (Austria, Fun World, bootleg)

    This one seems to have normal RAM instead of NVRAM.
    Going through the code, there's not any NVRAM initialization routine through service 1 & 2.



    - Mongolfier New
    - Soccer New

    These games are based in Royal Card. They are running in a heavely modified Royal Card
    hardware with a microcontroller as an extra (protection?) component and a TDA2003 as
    audio amplifier.

    The extra microcontroller is a 8 bit (PLCC-44) TSC87C52-16CB from Intel (now dumped!)

    Each set has double sized ROMs. One half contains the proper set and the other half store
    a complete Royal Card set, so... Is possible the existence of a shortcut,'easter egg',
    simple hack or DIP switches combination to enable the Royal Card game.



***********************************************************************************


    Memory Map (generic)
    --------------------


    $0000 - $07FF   NVRAM           // All registers and settings.
    $0800 - $0803   PIA1            // Input Ports 0 & 1.
    $0A00 - $0A03   PIA2            // Input Ports 2 & 3.
    $0C00 - $0C00   AY-8910 (R/C)   // Read/Control.
    $0C01 - $0C01   AY-8910 (W)     // Write.
    $0E00 - $0E00   CRTC6845 (A)    // MC6845 adressing.
    $0E01 - $0E01   CRTC6845 (R/W)  // MC6845 Read/Write.

    $2000 - $2FFF   VideoRAM (funworld/bigdeal)
    $3000 - $3FFF   ColorRAM (funworld/bigdeal)

    $4000 - $4FFF   VideoRAM (magiccrd/royalcrd)
    $5000 - $5FFF   ColorRAM (magiccrd/royalcrd)

    $6000 - $6FFF   VideoRAM (cuoreuno/elephfam)
    $7000 - $7FFF   ColorRAM (cuoreuno/elephfam)

    $8000 - $FFFF   ROM (almost all games)



    *** MC6845 Initialization ***

    ----------------------------------------------------------------------------------------------------------------------
    register:   00    01    02    03    04    05    06    07    08    09    10    11    12    13    14    15    16    17
    ----------------------------------------------------------------------------------------------------------------------
    jollycrd:  0x7C  0x60  0x65  0x08  0x1E  0x08  0x1D  0x1D  0x00  0x07  0x01  0x01  0x00  0x00  #$00  #$00  #$00  #$00.
    jolycdae:  0x7C  0x60  0x65  0x08  0x1E  0x08  0x1D  0x1D  0x00  0x07  0x01  0x01  0x00  0x00  #$00  #$00  #$00  #$00.
    jolycdcr:  0x7C  0x60  0x65  0x08  0x1E  0x08  0x1D  0x1D  0x00  0x07  0x01  0x01  0x00  0x00  #$00  #$00  #$00  #$00.
    jolycdit:  0x7C  0x60  0x65  0x08  0x1E  0x08  0x1D  0x1D  0x00  0x07  0x01  0x01  0x00  0x00  #$00  #$00  #$00  #$00.
    jolycdab:  0x7C  0x60  0x65  0x08  0x1E  0x08  0x1D  0x1D  0x00  0x07  0x01  0x01  0x00  0x00  #$00  #$00  #$00  #$00.

    bigdeal:   0x7C  0x60  0x65  0x08  0x1E  0x08  0x1D  0x1D  0x00  0x07  0x01  0x01  0x00  0x00  #$00  #$00  #$00  #$00.

    cuoreuno:  0x7C  0x60  0x65  0x08  0x1E  0x08  0x1D  0x1D  0x00  0x07  0x01  0x01  0x00  0x00  #$00  #$00  #$00  #$00.
    elephfam:  0x7C  0x60  0x65  0x08  0x1E  0x08  0x1D  0x1D  0x00  0x07  0x01  0x01  0x00  0x00  #$00  #$00  #$00  #$00.
    pool10:    0x7C  0x60  0x65  0x08  0x1E  0x08  0x1D  0x1D  0x00  0x07  0x01  0x01  0x00  0x00  #$00  #$00  #$00  #$00.
    tortufam:  0x7C  0x60  0x65  0x08  0x1E  0x08  0x1D  0x1D  0x00  0x07  0x01  0x01  0x00  0x00  #$00  #$00  #$00  #$00.

    royalcrd:  0x7C  0x60  0x65  0xA8  0x1E  0x08  0x1D  0x1C  0x00  0x07  0x01  0x01  0x00  0x00  #$00  #$00  #$00  #$00.
    magiccrd:  0x7B  0x70  0x66  0xA8  0x24  0x08  0x22  0x22  0x00  0x07  0x01  0x01  0x00  0x00  #$00  #$00  #$00  #$00.

    monglfir:  0x7C  0x60  0x65  0xA8  0x1E  0x08  0x1D  0x1C  0x00  0x07  0x01  0x01  0x00  0x00  #$00  #$00  #$00  #$00.
    soccernw:  0x7C  0x60  0x65  0xA8  0x1E  0x08  0x1D  0x1C  0x00  0x07  0x01  0x01  0x00  0x00  #$00  #$00  #$00  #$00.


***********************************************************************************


    *** Hardware Info ***


    Jolly Card, TAB Austria
    -----------------------

    Pinouts:

    X1-01   GND                     X1-A    GND
    X1-02   GND                     X1-B    GND
    X1-03   GND                     X1-C    GND
    X1-04   +5V                     X1-D    +5V
    X1-05   +12V                    X1-E    +12V
    X1-06   NC                      X1-F    NC
    X1-07   NC                      X1-G    NC
    X1-08   NC                      X1-H    NC
    X1-09   NC                      X1-I    Coin 1
    X1-10   Coin 2                  X1-J    Pay Out SW
    X1-11   Hold 3                  X1-K    NC
    X1-12   NC                      X1-L    GND
    X1-13   Hold 4                  X1-M    Remote
    X1-14   Bookkeeping SW          X1-N    GND
    X1-15   Hold 2                  X1-O    Cancel
    X1-16   Hold 1                  X1-P    Hold 5
    X1-17   Start                   X1-Q
    X1-18   Hopper Out              X1-R
    X1-19   Red                     X1-S    Green
    X1-20   Blue                    X1-T    Sync
    X1-21   GND                     X1-U    Speaker GND
    X1-22   Speaker +               X1-V    Management SW

    X2-01   GND                     X2-A    GND
    X2-02   NC                      X2-B    NC
    X2-03   Start                   X2-C    NC
    X2-04   NC                      X2-D    NC
    X2-05   NC                      X2-E    NC
    X2-06   Lamp Start              X2-F    Lamp Hold 1+3
    X2-07   Lamp Hold 2             X2-G    Lamp Hold 4
    X2-08   Lamp Hold 5             X2-H    Lamp Cancel
    X2-09   NC                      X2-I    NC
    X2-10   Counter In              X2-J    Counter Out
    X2-11   Hopper Count            X2-K    Hopper Drive
    X2-12   Counter Remote          X2-L
    X2-13                           X2-M
    X2-14                           X2-N
    X2-15   NC                      X2-O
    X2-16                           X2-P
    X2-17                           X2-Q    Coin Counter
    X2-18                           X2-R

    ---------------------------------------------------

    DIP Switches:

        ON                  OFF

    1   Hopper              Manual Payout SW    :Payout
    2   Auto Hold           No Auto Hold        :Hold
    3   Joker               Without Joker       :Joker
    4   Dattl Insert        TAB Insert          :Inserts
    5   5 Points/Coin       10 Points/Coin      :Coin 1
    6   5 Points/Coin       10 Points/Coin      :Coin 2
    7   10 Points/Pulse     100 Points/Pulse    :Remote
    8   Play                Keyboard Test       :Mode

    ---------------------------------------------------


    Jolly Card (Austria, Fun World, bootleg)
    ----------------------------------------

    - 1x G65SC02P (CPU)
    - 1x MC68B45P (CRTC)
    - 1x AY3-8910 (sound)
    - 2x MC6821P  (PIAs)

    RAM:  - 1x 6116
          - 1x KM6264AL-10

    - 1x Crystal : 16.000 Mhz


    Jolly Card (other)
    ------------------

    - 1x G65SC02P (CPU)
    - 1x MC68B45P (CRTC)
    - 1x AY3-8910 (sound)
    - 2x MC6821P  (PIAs)

    RAM:  - 1x NVram DS1220Y (instead of 6116)
          - 1x KM6264AL-10

    - 1x Crystal : 16.000 Mhz


    Jolly Card (Italia)
    -------------------

    - 1x HY6264LP
    - 1x MC6845P
    - 1x HM6116LP
    - 1x G65SC02 (main)
    - 1x AY-3-8910 (sound)
    - 1x MC6821P
    - 1x oscillator 16.000 MHz
    - ROMs  2x TMS27c256 (1,2)
            1x M5M27256 (jn)
    - 1x prom N82S147
    - 1x GAL16V8B
    - 1x 8 DIP switches
    - 1x 22x2 edge connector
    - 1x 18x2 edge connector
    - 1x trimmer (volume)(missing)


    Big Deal (Hungary)
    ------------------

    - 1x MC6845P
    - 1x YM2149F
    - 2x MC6821P
    - 1x Crystal 16.000


    Magic Card II (Bulgaria, bootleg)
    ---------------------------------

    - 1x Special CPU with CM602 (??) on it
    - 1x MC6845P
    - 1x YM2149F
    - 2x MC6821P
    - 1x Crystal 16.000


    Cuore Uno (Italia)
    ------------------

    - CPU 1x G65SC02P
    - 1x MC68B45P (CRT controller)
    - 2x MC68B21CP (Peripheral Interface Adapter)
    - 1x unknown (95101) DIP40 mil600
    - 1x oscillator 16.000000MHz
    - ROMs  1x TMS27C512
            2x TMS27C256
    - 1x PROM AM27S29
    - 2x PALCE20V8H
    - 1x PALCE16V8H (soldered)
    - Note 1x JAMMA edge connector (keep -5 disconnected)
    - 1x trimmer (volume)
    - 1x 8 DIP switches
    - 1x battery


    Elephant Family (Italia, old)
    -----------------------------

    - CPU 1x R65C02P2
    - 1x MC68B45P (CRT controller)
    - 2x EF6821P (Peripheral Interface Adapter)
    - 1x unknown (95101) DIP40 mil600
    - 1x oscillator 16.000MHz
    - ROMs  2x M27C256
            1x TMS27C256
    - 1x PROM AM27S29
    - 2x PALCE20V8H (read protected)
    - 1x PALCE16V8H (read protected)
    - Note 1x JAMMA edge connector (keep -5 disconnected)
    - 1x trimmer (volume)
    - 1x 8 DIP switches
    - 1x battery


    Pool 10 (Italia)
    ----------------

    - 1x R65C02P2 (main)
    - 1x YM2149F (sound)
    - 1x HD46505 (CRC controller)
    - 2x EF6821P (Peripheral Interface Adapter)
    - 1x oscillator 16.000000MHz

    - 2x M27256 (pool,1)
    - 1x D27256 (2)
    - 1x PROM N82S147AN
    - 2x GAL20V8B (read protected)
    - 1x PALCE16V8H (read protected)

    - 1x JAMMA edge connector
    - 1x trimmer (volume)
    - 1x 8 DIP switches
    - 1x battery


    Tortuga Family (Italia)
    -----------------------

    - 1x G65SC02P2 (main)
    - 1x 95101 (sound)
    - 1x MC68B45P (CRC controller)
    - 2x MC68B21CP (Peripheral Interface Adapter)
    - 1x oscillator 16.000000MHz

    - 3x TMS27C256
    - 1x PROM AM27S29PC
    - 2x PALCE20V8H (read protected)
    - 1x PALCE16V8H (read protected)

    - 1x JAMMA edge connector
    - 1x trimmer (volume)
    - 1x 8 DIP switches
    - 1x battery


    Royal Card (set 1)
    ------------------

    - 1x HM6264LP
    - 1x HD4650SP
    - 1x HM6116LP
    - 1x R65C02P2 (main)
    - 1x WB5300 (labeled YM8910)(sound)
    - 1x EF6821P
    - 1x oscillator 16.000 MHz

    - 1x D27256 (1)
    - 1x S27C256 (2)
    - 1x TMS27C256 (r2)
    - 2x PEEL18CV8 (1 protected)
    - 1x PALCE16V8H (protected)
    - 1x PROM N82S147AN

    - 1x 8 DIP Switches
    - 1x 22x2 edge connector
    - 1x 18x2 edge connector
    - 1x trimmer (volume)


    Royal Card (set 2)
    ------------------

    - CPU 1x R65C02P2 (main)
    - 1x MC68B45P (CRT controller)
    - 2x MC68B21CP (Peripheral Interface Adapter)
    - 1x oscillator 16.000MHz
    - 3x ROMs TMS 27C512
    - 1x PALCE16V8H
    - 1x prom AM27S29APC

    - 1x 28x2 connector (maybe NOT jamma)
    - 1x 10x2 connector
    - 1x 3 legs connector (solder side)
    - 1x 8 DIP Switches
    - 1x trimmer (volume)


    Royal Card (set 3, encrypted)
    -----------------------------

    - Custom/encrypted CPU (epoxy block labelled "EVONA EX9511" -> www.evona.sk )
        inserted into socked with "6502" mark.

    - 1x YM2149

    - 1x HD6845 (CRTC)
    - 1x MC68A21P (PIA)
    - 1x 40 pin IC with surface scratched (PIA)
    - 1x 8 DIP Switches
    - Sanyo LC3517B SRAM (videoram ?)
    - 6264 battery backed SRAM (battery is dead)
    - 1x PALCE16V8
    - 1x GAL16V8B
    - 1x PEEL18CV8P x2
    - 1x 82S147 PROM (near Yamaha and unknown 40pin) - "82s147.bin"
    - 1x 27256 close to CPU module - "1.bin"
    - 2x 27256 - gfx - "2.bin", "3.bin"


    Mongolfier New
    --------------

    - 1x G65SC02P2 (main)
    - 1x KC89C72 (sound)
    - 1x TDA2003 (sound)
    - 1x MC68B45P (CRC controller)
    - 2x EF6821P (Peripheral Interface Adapter)
    - 1x TSC87C52-16CB (PLCC44)(Programmable 8bit Microcontroller)(not dumped)
    - 1x M48Z08-100PC1 (Zero Power RAM - Lithium Battery)
    - 1x oscillator 16.0000MHz

    - 3x M27C512
    - 1x PROM AM27S29PC
    - 1x PALCE16V8H (read protected)

    - 1x JAMMA edge connector
    - 1x trimmer (volume)
    - 2x 8 DIP switches
    - 1x 4 DIP switches
    - 1x green led


    Soccer New (Italia)
    -------------------

    - 1x G65SC02P2 (main)
    - 1x KC89C72 (sound)
    - 1x TDA2003 (sound)
    - 1x MC68B45P (CRC controller)
    - 1x EF68B21P (Peripheral Interface Adapter)
    - 1x EF6821P (Peripheral Interface Adapter)
    - 1x TSC87C52-16CB (PLCC44)(Programmable 8bit Microcontroller)(not dumped)
    - 1x M48Z08-100PC1 (Zero Power RAM - Lithium Battery)
    - 1x oscillator 16.0000MHz

    - 3x M27C512
    - 1x PROM AM27S29PC
    - 1x PALCE16V8H (read protected)

    - 1x JAMMA edge connector
    - 1x trimmer (volume)
    - 2x 8 DIP switches
    - 1x 4 DIP switches
    - 1x green led


***********************************************************************************


    *** Driver updates by Roberto Fresca ***


    2005/09/08
    - Added Cuore Uno, Elephant Family and Royal Card.

    2005/09/19
    - Added some clones.
    - Cleaned up and renamed all sets. Made parent-clone relationship.

    2005/12/15
    - Corrected CPU freq (2 MHz) in cuoreuno and elephfam (both have R65c02P2).
      (I suspect more games must have their CPU running at 2 MHz).
    - Corrected videoram and colorram offset in cuoreuno and elephfam.
    - To initialize the NVRAM in cuoreuno and elephfam:
      Start game, press and hold Service1 & Service2, press reset (F3),
      release Service1 & Service2 and press reset (F3) again.

    2006/10/18
    - Corrected the screen size and visible area to cuoreuno and elephfam based on mc6845 registers.
    - Added all inputs to cuoreuno and elephfam.
    - Added test mode DIP switch to cuoreuno and elephfam.
    - Managed cuoreuno and elephfam inputs to pass the initial checks. Now both games are playable.
    - Changed the cuoreuno full name to "Cuore 1" (as shown in the attract).

    2006/10/22-28
    - Corrected cuoreuno and elephfam graphics to 4bpp.
    - Fixed elephfam gfx planes.
    - Simulated cuoreuno palette based on screenshots.
    - Simulated elephfam palette based on screenshots.


    2006/11/01 to 2006/12/04

    ******** REWRITE ********

    - Merged/splitted some machine drivers, memory maps and inputs.
    - Unified get_bg_tile_info for all games.
    - Mapped the input buttons in a better format (all games). Keys: 156-QW-ZXCVBNM.
    - Added proper color PROM decode routines.
    - Rewrote the technical notes.
    - Splitted the driver to driver/video.

    - Corrected the screen size and visible area to magiccrd based on mc6845 registers.
    - Added the remaining 2 GFX planes to magiccrd, but GFX are imperfect (bad decode or bad dump?). Color PROM need to be dumped.
    - Royalcrd: Added all inputs and DIP switches.
                Fixed memory map, gfx decode
                Corrected screen size and visible area based on mc6845 registers.
                Corrected CPU clock to 2mhz.

    - New game added: Joker Poker. Not working due to use of custom encrypted CPU.
    - New game added: Royal Card (Slovakia, encrypted). Not working due to use of a custom encrypted CPU.
    - Fixed jolycdcr gfx to 4bpp.
    - Other fixes to get jolycdcr running.
    - Managed royalcdb to work, using the 2nd half of program ROM. (seems to be mapped that way)
    - Managed jolycdit to work, but with imperfect graphics due to gfx encryption.
    - Fixed CPU clock to 2MHz. in all remaining games.
    - Fixed ay8910 frequency based on elephfam audio.
    - Fixed ay8910 volume in all games to avoid clipping.
    - Reworked jolycdcr inputs: The game was designed to work only with remote credits. After nvram init, set the payout dip to "manual".
    - Reworked jolycdit inputs: After nvram init, set the payout dip to "manual" to allow work the remote mode.
    - Set jolycdat as bigdeal clone. The game has the same layout/behaviour instead of the normal jolly card games, even when are sharing gfx roms.
    - Added the bipolar PROM and GAL to jolycdit. Confirmed the GFX ROMs as good dumps.
    - Added an alternate set of Elephant Family. This one lacks of test mode and doesn't allow to switch between min-max bets through stop1.
    - Added color PROMs to cuoreuno and elephfam sets but still no routed. Also added PLDs (protected, bad dumps).
    - Corrected jollycrd screen size and visible area based on mc6845 registers.
    - Hooked, wired and decoded the color prom in jollycrd sets based on jolycdit redump. Now colors are perfect.
    - Wired and decoded the color prom in cuoreuno and elephfam sets. Now colors are perfect.
    - Wired and decoded the color prom in royalcrd. Now colors are perfect.
    - Hooked, wired and decoded the color prom in bigdeal sets based on jolycdat (jollycrd palette).
      Colors seems to be correct, but need to check against the real thing. Flagged as IMPERFECT_COLORS till a color PROM dump appear.
    - Decrypted jolycdit gfx roms.
    - Added set Jolly Card (Austria, encrypted).
    - Decrypted jolycdae and managed the planes to show correct colors. The set is working properly.


    2006/12/24
    - Fixed some incomplete inputs.
    - Added new working game: Pool 10.
    - Added new working game: Tortuga Family.
    - Added new game: Mongolfier New. Not working due to the lack of MCU emulation.
    - Added new game: Soccer New. Not working due to the lack of MCU emulation.
    - Updated technical notes.

    2007/02/01
    - All crystals documented via #defines.
    - All CPU and sound clocks derived from #defined crystal values.
    - Added DIPLOCATIONS to all games.
    - Added a pool10 alternate set.
    - Added proper tsc87c52 MCU dumps to monglfir and soccernew.
    - Modified the refresh rate to 60 fps according to some video evidences.
    - Updated technical notes.



    *** TO DO ***

    - Fix the last 2 GFX planes in magiccrd.
    - Figure out the royalcdc and jokercrd encryption.
    - Analyze the unknown writes to $2000/$4000 in some games.
    - Check for the reads to the ay8910 output ports in some games.
    - Figure out the MCU in monglfir and soccernw.
    - Correct the ROM_REGION in some games to allow the use of RGN_FRAC


***********************************************************************************/


#define MASTER_CLOCK	16000000

#include "driver.h"
#include "sound/ay8910.h"
#include "video/crtc6845.h"
#include "machine/6821pia.h"

#include "funworld.lh"

/* from video */
extern WRITE8_HANDLER( funworld_videoram_w );
extern WRITE8_HANDLER( funworld_colorram_w );
extern PALETTE_INIT( funworld );
extern VIDEO_START( funworld );
extern VIDEO_START( magiccrd );
extern VIDEO_UPDATE( funworld );


/**********************
* Read/Write Handlers *
**********************/

WRITE8_HANDLER(funworld_lamp_a_w)
{
	coin_counter_w(0, data & 0x01);		// credit in counter

	output_set_lamp_value(0, 1-((data >> 1) & 1));	// button hold1 and
	output_set_lamp_value(2, 1-((data >> 1) & 1));	// hold3 (see pinouts)

	coin_counter_w(7, data & 0x04);		// credit out counter, mapped as coin 8

	output_set_lamp_value(1, 1-((data >> 3) & 1));	// button hold2/low
	output_set_lamp_value(5, 1-((data >> 5) & 1));	// button 6 (collect/cancel)
	output_set_lamp_value(3, (data >> 7) & 1);		// button hold4/high
}

WRITE8_HANDLER(funworld_lamp_b_w)
{
	output_set_lamp_value(4, (data >> 0) & 1);		// button hold5/bet
	output_set_lamp_value(6, (data >> 1) & 1);		// button 7 (start/play)
}


/*************************
* Memory map information *
*************************/

static ADDRESS_MAP_START( funworld_map, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x07ff) AM_RAM AM_BASE(&generic_nvram) AM_SIZE(&generic_nvram_size)
	AM_RANGE(0x0800, 0x0803) AM_READWRITE(pia_0_r, pia_0_w)
	AM_RANGE(0x0a00, 0x0a03) AM_READWRITE(pia_1_r, pia_1_w)
	AM_RANGE(0x0c00, 0x0c00) AM_READWRITE(AY8910_read_port_0_r, AY8910_control_port_0_w)
	AM_RANGE(0x0c01, 0x0c01) AM_WRITE(AY8910_write_port_0_w)
	AM_RANGE(0x0e00, 0x0e00) AM_WRITE(crtc6845_address_w)
	AM_RANGE(0x0e01, 0x0e01) AM_READWRITE(crtc6845_register_r, crtc6845_register_w)
	AM_RANGE(0x2000, 0x2fff) AM_RAM AM_WRITE(funworld_videoram_w) AM_BASE(&videoram)
	AM_RANGE(0x3000, 0x3fff) AM_RAM	AM_WRITE(funworld_colorram_w) AM_BASE(&colorram)
	AM_RANGE(0x4000, 0x4000) AM_READNOP
	AM_RANGE(0x8000, 0xbfff) AM_ROM
	AM_RANGE(0xc000, 0xffff) AM_ROM
ADDRESS_MAP_END

static ADDRESS_MAP_START( magiccrd_map, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x07ff) AM_RAM AM_BASE(&generic_nvram) AM_SIZE(&generic_nvram_size)
	AM_RANGE(0x0800, 0x0803) AM_READWRITE(pia_0_r, pia_0_w)
	AM_RANGE(0x0a00, 0x0a03) AM_READWRITE(pia_1_r, pia_1_w)
	AM_RANGE(0x0c00, 0x0c00) AM_READWRITE(AY8910_read_port_0_r, AY8910_control_port_0_w)
	AM_RANGE(0x0c01, 0x0c01) AM_WRITE(AY8910_write_port_0_w)
	AM_RANGE(0x0e00, 0x0e00) AM_WRITE(crtc6845_address_w)
	AM_RANGE(0x0e01, 0x0e01) AM_READWRITE(crtc6845_register_r, crtc6845_register_w)
	AM_RANGE(0x3600, 0x36ff) AM_RAM	// some games use $3603-05 range for protection.
	AM_RANGE(0x4000, 0x4fff) AM_RAM AM_WRITE(funworld_videoram_w) AM_BASE(&videoram)
	AM_RANGE(0x5000, 0x5fff) AM_RAM	AM_WRITE(funworld_colorram_w) AM_BASE(&colorram)
	AM_RANGE(0x6000, 0xffff) AM_ROM
ADDRESS_MAP_END

static ADDRESS_MAP_START( cuoreuno_map, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x07ff) AM_RAM AM_BASE(&generic_nvram) AM_SIZE(&generic_nvram_size)
	AM_RANGE(0x0800, 0x0803) AM_READWRITE(pia_0_r, pia_0_w)
	AM_RANGE(0x0a00, 0x0a03) AM_READWRITE(pia_1_r, pia_1_w)
	AM_RANGE(0x0c00, 0x0c00) AM_READWRITE(AY8910_read_port_0_r, AY8910_control_port_0_w)
	AM_RANGE(0x0c01, 0x0c01) AM_WRITE(AY8910_write_port_0_w)
	AM_RANGE(0x0e00, 0x0e00) AM_WRITE(crtc6845_address_w)
	AM_RANGE(0x0e01, 0x0e01) AM_READWRITE(crtc6845_register_r, crtc6845_register_w)
	AM_RANGE(0x2000, 0x2000) AM_READNOP	// some unknown reads
	AM_RANGE(0x3e00, 0x3fff) AM_RAM	// some games use $3e03-05 range for protection.
	AM_RANGE(0x6000, 0x6fff) AM_RAM AM_WRITE(funworld_videoram_w) AM_BASE(&videoram)
	AM_RANGE(0x7000, 0x7fff) AM_RAM	AM_WRITE(funworld_colorram_w) AM_BASE(&colorram)
	AM_RANGE(0x8000, 0xffff) AM_ROM
ADDRESS_MAP_END

static ADDRESS_MAP_START( royalmcu_map, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x07ff) AM_RAM AM_BASE(&generic_nvram) AM_SIZE(&generic_nvram_size)
	AM_RANGE(0x2800, 0x2803) AM_READWRITE(pia_0_r, pia_0_w)
	AM_RANGE(0x2a00, 0x2a03) AM_READWRITE(pia_1_r, pia_1_w)
	AM_RANGE(0x2c00, 0x2c00) AM_READWRITE(AY8910_read_port_0_r, AY8910_control_port_0_w)
	AM_RANGE(0x2c01, 0x2c01) AM_WRITE(AY8910_write_port_0_w)
	AM_RANGE(0x2e00, 0x2e00) AM_WRITE(crtc6845_address_w)
	AM_RANGE(0x2e01, 0x2e01) AM_READWRITE(crtc6845_register_r, crtc6845_register_w)
	AM_RANGE(0x4000, 0x4fff) AM_RAM AM_WRITE(funworld_videoram_w) AM_BASE(&videoram)
	AM_RANGE(0x5000, 0x5fff) AM_RAM	AM_WRITE(funworld_colorram_w) AM_BASE(&colorram)
	AM_RANGE(0x6000, 0xffff) AM_ROM
ADDRESS_MAP_END


/*************************
*      Input ports       *
*************************/

INPUT_PORTS_START( funworld )
	PORT_START_TAG("IN0")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_SERVICE )	PORT_NAME("Remote") PORT_CODE(KEYCODE_Q)
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON1 )	PORT_NAME("Halten (Hold) 1") PORT_CODE(KEYCODE_Z)
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_BUTTON6 )	PORT_NAME("Loeschen (Cancel) / Kassieren (Take)") PORT_CODE(KEYCODE_N)
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_START1 )		PORT_NAME("Geben (Start) / Gamble (Play)")
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON5 )	PORT_NAME("Halten (Hold) 5 / Half Gamble") PORT_CODE(KEYCODE_B)
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_SERVICE1 )	PORT_NAME("Buchhalt (Service1)")
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_SERVICE2 )	PORT_NAME("Einstellen (Service2)")
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_BUTTON4 )	PORT_NAME("Halten (Hold) 4 / Hoch (High)") PORT_CODE(KEYCODE_V)

	PORT_START_TAG("IN1")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_BUTTON2 )	PORT_NAME("Halten (Hold) 2 / Tief (Low)") PORT_CODE(KEYCODE_X)
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON3 )	PORT_NAME("Halten (Hold) 3") PORT_CODE(KEYCODE_C)
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_SERVICE )	PORT_NAME("Hoppersch") PORT_CODE(KEYCODE_W)
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_BUTTON7 )	PORT_NAME("Abschreib (Payout)") PORT_CODE(KEYCODE_M)	// Payout? Need to check with hopper filled.

	PORT_START_TAG("IN2")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START_TAG("DSW")
	PORT_DIPNAME( 0x01, 0x01, "State" )				PORT_DIPLOCATION("SW1:8")
	PORT_DIPSETTING(    0x00, "Keyboard Test" )
	PORT_DIPSETTING(    0x01, "Play" )
	PORT_DIPNAME( 0x02, 0x00, "Remote" )			PORT_DIPLOCATION("SW1:7")
	PORT_DIPSETTING(    0x00, "10 Points/Pulse" )
	PORT_DIPSETTING(    0x02, "100 Points/Pulse" )
	PORT_DIPNAME( 0x04, 0x00, DEF_STR( Coin_B ) )	PORT_DIPLOCATION("SW1:6")
	PORT_DIPSETTING(    0x00, "5 Points/Coin" )
	PORT_DIPSETTING(    0x04, "10 Points/Coin" )
	PORT_DIPNAME( 0x08, 0x00, DEF_STR( Coin_A ) )	PORT_DIPLOCATION("SW1:5")
	PORT_DIPSETTING(    0x00, "5 Points/Coin" )
	PORT_DIPSETTING(    0x08, "10 Points/Coin" )
	PORT_DIPNAME( 0x10, 0x00, "Insert" )			PORT_DIPLOCATION("SW1:4")
	PORT_DIPSETTING(    0x00, "Dattl Insert" )
	PORT_DIPSETTING(    0x10, "TAB Insert" )
	PORT_DIPNAME( 0x20, 0x00, "Joker" )				PORT_DIPLOCATION("SW1:3")
	PORT_DIPSETTING(    0x00, "With Joker" )		// also enable Five of a Kind.
	PORT_DIPSETTING(    0x20, "Without Joker" )
	PORT_DIPNAME( 0x40, 0x00, "Hold" )				PORT_DIPLOCATION("SW1:2")
	PORT_DIPSETTING(    0x00, "Auto Hold" )
	PORT_DIPSETTING(    0x40, "No Auto Hold" )

	/* after nvram init, set the following one to 'manual'
    to allow the remote credits mode to work */

	PORT_DIPNAME( 0x80, 0x00, "Payout" )			PORT_DIPLOCATION("SW1:1")
	PORT_DIPSETTING(    0x00, "Hopper" )
	PORT_DIPSETTING(    0x80, "Manual Payout SW" )
INPUT_PORTS_END

INPUT_PORTS_START( jolycdcr )
	PORT_START_TAG("IN0")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_SERVICE )	PORT_NAME("Navijanje (Remote)") PORT_CODE(KEYCODE_Q)
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON1 )	PORT_NAME("Stop (Hold) 1") PORT_CODE(KEYCODE_Z)
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_BUTTON6 )	PORT_NAME("Ponistavange (Cancel) / Kasiranje (Take) / Autohold") PORT_CODE(KEYCODE_N)
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_START1 )		PORT_NAME("Djelenje (Start) / Gamble (Play)")
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON5 )	PORT_NAME("Stop (Hold) 5 / Ulog (Bet) / Half Gamble") PORT_CODE(KEYCODE_B)
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_SERVICE1 )	PORT_NAME("Konobar (Service1)")
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_SERVICE2 )	PORT_NAME("Namjestit (Service2)")
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_BUTTON4 )	PORT_NAME("Stop (Hold) 4 / Veca (High)") PORT_CODE(KEYCODE_V)

	PORT_START_TAG("IN1")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_BUTTON2 )	PORT_NAME("Stop (Hold) 2 / Manja (Low)") PORT_CODE(KEYCODE_X)
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON3 )	PORT_NAME("Stop (Hold) 3") PORT_CODE(KEYCODE_C)
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_BUTTON7 )	PORT_NAME("Vratiti Nazad (Payout)") PORT_CODE(KEYCODE_M)	// Payout? Need to check with hopper filled.

	PORT_START_TAG("IN2")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START_TAG("DSW")
	PORT_DIPNAME( 0x01, 0x01, "State" )				PORT_DIPLOCATION("SW1:8")
	PORT_DIPSETTING(    0x00, "Keyboard Test" )
	PORT_DIPSETTING(    0x01, "Play" )
	PORT_DIPNAME( 0x02, 0x00, "Remote" )			PORT_DIPLOCATION("SW1:7")
	PORT_DIPSETTING(    0x00, "10 Points/Pulse" )
	PORT_DIPSETTING(    0x02, "100 Points/Pulse" )
	PORT_DIPNAME( 0x04, 0x00, DEF_STR( Unknown ) )	PORT_DIPLOCATION("SW1:6")
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x04, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x00, DEF_STR( Unknown ) )	PORT_DIPLOCATION("SW1:5")
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x08, DEF_STR( On ) )
	PORT_DIPNAME( 0x10, 0x00, DEF_STR( Unknown ) )	PORT_DIPLOCATION("SW1:4")
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x10, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x00, "Joker" )				PORT_DIPLOCATION("SW1:3")
	PORT_DIPSETTING(    0x00, "With Joker" )		// also enable Five of a Kind.
	PORT_DIPSETTING(    0x20, "Without Joker" )
	PORT_DIPNAME( 0x40, 0x00, "Hold" )				PORT_DIPLOCATION("SW1:2")
	PORT_DIPSETTING(    0x00, "Auto Hold" )
	PORT_DIPSETTING(    0x40, "No Auto Hold" )

	/* after nvram init, set the following one to 'manual'
    to allow the remote credits mode to work */

	PORT_DIPNAME( 0x80, 0x00, "Payout" )			PORT_DIPLOCATION("SW1:1")
	PORT_DIPSETTING(    0x00, "Hopper" )
	PORT_DIPSETTING(    0x80, "Manual Payout SW" )
INPUT_PORTS_END

INPUT_PORTS_START( jolycdit )
	PORT_START_TAG("IN0")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_SERVICE )	PORT_NAME("Remote") PORT_CODE(KEYCODE_Q)
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON1 )	PORT_NAME("Stop (Hold) 1 / Alta (High)") PORT_CODE(KEYCODE_Z)
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_BUTTON6 )	PORT_NAME("Clear / Doppio (Double) / Autohold") PORT_CODE(KEYCODE_N)
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_START1 )		PORT_NAME("Start")
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON5 )	PORT_NAME("Stop (Hold) 5 / Half Gamble") PORT_CODE(KEYCODE_B)
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_SERVICE2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_BUTTON4 )	PORT_NAME("Stop (Hold) 4 / Accredito (Take)") PORT_CODE(KEYCODE_V)

	PORT_START_TAG("IN1")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_BUTTON2 )	PORT_NAME("Stop (Hold) 2") PORT_CODE(KEYCODE_X)
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON3 )	PORT_NAME("Stop (Hold) 3 / Bassa (Low)") PORT_CODE(KEYCODE_C)
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_BUTTON7 )	PORT_NAME("Payout") PORT_CODE(KEYCODE_M)

	PORT_START_TAG("IN2")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START_TAG("DSW")
	PORT_DIPNAME( 0x01, 0x01, "State" )				PORT_DIPLOCATION("SW1:8")
	PORT_DIPSETTING(    0x00, "Keyboard Test" )
	PORT_DIPSETTING(    0x01, "Play" )
	PORT_DIPNAME( 0x02, 0x00, "Remote" )			PORT_DIPLOCATION("SW1:7")
	PORT_DIPSETTING(    0x00, "10 Points/Pulse" )
	PORT_DIPSETTING(    0x02, "50 Points/Pulse" )
	PORT_DIPNAME( 0x0c, 0x00, DEF_STR( Coin_A ) )	PORT_DIPLOCATION("SW1:5,6")
	PORT_DIPSETTING(    0x00, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x0c, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x04, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x08, DEF_STR( 1C_5C ) )
	PORT_DIPNAME( 0x10, 0x00, DEF_STR( Unknown ) )	PORT_DIPLOCATION("SW1:4")
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x00, "Joker" )				PORT_DIPLOCATION("SW1:3")
	PORT_DIPSETTING(    0x00, "With Joker" )		// also enable Five of a Kind.
	PORT_DIPSETTING(    0x20, "Without Joker" )
	PORT_DIPNAME( 0x40, 0x00, "Hold" )				PORT_DIPLOCATION("SW1:2")
	PORT_DIPSETTING(    0x00, "Auto Hold" )
	PORT_DIPSETTING(    0x40, "No Auto Hold" )

	/* after nvram init, set the following one to 'manual'
    to allow the remote credits mode to work */

	PORT_DIPNAME( 0x80, 0x00, "Payout" )			PORT_DIPLOCATION("SW1:1")
	PORT_DIPSETTING(    0x00, "Hopper" )
	PORT_DIPSETTING(    0x80, "Manual Payout SW" )
INPUT_PORTS_END

INPUT_PORTS_START( bigdeal )
	PORT_START_TAG("IN0")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_SERVICE ) PORT_NAME("Remote") PORT_CODE(KEYCODE_Q)
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_NAME("Hold 1") PORT_CODE(KEYCODE_Z)
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_BUTTON6 ) PORT_NAME("Clear / Take") PORT_CODE(KEYCODE_N)
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_START1 )  PORT_NAME("Start")
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON5 ) PORT_NAME("Hold 5 / Stake") PORT_CODE(KEYCODE_B)
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_SERVICE2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_BUTTON4 ) PORT_NAME("Hold 4 / Nagy (High)") PORT_CODE(KEYCODE_V)

	PORT_START_TAG("IN1")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_NAME("Hold 2 / Icsi (Low)") PORT_CODE(KEYCODE_X)
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON3 ) PORT_NAME("Hold 3 / Half Gamble") PORT_CODE(KEYCODE_C)
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_BUTTON7 ) PORT_NAME("Payout") PORT_CODE(KEYCODE_M)

	PORT_START_TAG("IN2")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START_TAG("DSW")
	/* the following one should be left ON by default to allow initialization */
	PORT_DIPNAME( 0x01, 0x01, DEF_STR( Unknown ) )	PORT_DIPLOCATION("SW1:8")
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x01, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x00, DEF_STR( Unknown ) )	PORT_DIPLOCATION("SW1:7")
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x02, DEF_STR( On ) )
	PORT_DIPNAME( 0x04, 0x00, "Remote" )			PORT_DIPLOCATION("SW1:6")
	PORT_DIPSETTING(    0x00, "10 Points/Pulse" )
	PORT_DIPSETTING(    0x04, "100 Points/Pulse" )
	PORT_DIPNAME( 0x08, 0x00, DEF_STR( Coin_A ) )	PORT_DIPLOCATION("SW1:5")
	PORT_DIPSETTING(    0x00, "5 Points/Coin" )
	PORT_DIPSETTING(    0x08, "10 Points/Coin" )
	PORT_DIPNAME( 0x10, 0x00, DEF_STR( Unknown ) )	PORT_DIPLOCATION("SW1:4")
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x10, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x00, DEF_STR( Unknown ) )	PORT_DIPLOCATION("SW1:3")
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x20, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x00, "Hold" )				PORT_DIPLOCATION("SW1:2")
	PORT_DIPSETTING(    0x00, "Auto Hold" )
	PORT_DIPSETTING(    0x40, "No Auto Hold" )

	/* after nvram init, set the following one to 'manual'
    to allow the remote credits mode to work */

	PORT_DIPNAME( 0x80, 0x00, "Payout" )	PORT_DIPLOCATION("SW1:1")
	PORT_DIPSETTING(    0x00, "Hopper" )
	PORT_DIPSETTING(    0x80, "Manual Payout SW" )
INPUT_PORTS_END

INPUT_PORTS_START( magiccrd )
	PORT_START_TAG("IN0")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_SERVICE ) PORT_NAME("Remote") PORT_CODE(KEYCODE_Q)
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_NAME("Hold 1") PORT_CODE(KEYCODE_Z)
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_BUTTON6 ) PORT_NAME("Clear / Take") PORT_CODE(KEYCODE_N)
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_START1 )  PORT_NAME("Start")
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON5 ) PORT_NAME("Hold 5 / Stake") PORT_CODE(KEYCODE_B)
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_SERVICE2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_BUTTON4 ) PORT_NAME("Hold 4 / High") PORT_CODE(KEYCODE_V)

	PORT_START_TAG("IN1")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_NAME("Hold 2 / Low") PORT_CODE(KEYCODE_X)
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON3 ) PORT_NAME("Hold 3") PORT_CODE(KEYCODE_C)
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_BUTTON7 ) PORT_NAME("Payout") PORT_CODE(KEYCODE_M)

	PORT_START_TAG("IN2")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START_TAG("DSW")
	PORT_DIPNAME( 0x01, 0x01, "State" )				PORT_DIPLOCATION("SW1:8")
	PORT_DIPSETTING(    0x00, "Keyboard Test" )
	PORT_DIPSETTING(    0x01, "Play" )
	PORT_DIPNAME( 0x02, 0x00, DEF_STR( Unknown ) )	PORT_DIPLOCATION("SW1:7")	// remote credits settings are always 10 Points/Pulse.
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x02, DEF_STR( On ) )
	PORT_DIPNAME( 0x04, 0x00, DEF_STR( Coin_A ) )	PORT_DIPLOCATION("SW1:6")
	PORT_DIPSETTING(    0x00, "5 Points/Coin" )
	PORT_DIPSETTING(    0x04, "10 Points/Coin" )
	PORT_DIPNAME( 0x08, 0x00, DEF_STR( Coin_B ) )	PORT_DIPLOCATION("SW1:5")
	PORT_DIPSETTING(    0x00, "5 Points/Coin" )
	PORT_DIPSETTING(    0x08, "10 Points/Coin" )
	PORT_DIPNAME( 0x10, 0x00, DEF_STR( Unknown ) )	PORT_DIPLOCATION("SW1:4")
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x10, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x00, "Joker" )				PORT_DIPLOCATION("SW1:3")
	PORT_DIPSETTING(    0x00, "With Joker" )
	PORT_DIPSETTING(    0x20, "Without Joker" )
	PORT_DIPNAME( 0x40, 0x00, "Hold" )				PORT_DIPLOCATION("SW1:2")
	PORT_DIPSETTING(    0x00, "Auto Hold" )
	PORT_DIPSETTING(    0x40, "No Auto Hold" )

	/* after nvram init, set the following one to 'manual'
    to allow the remote credits mode to work */

	PORT_DIPNAME( 0x80, 0x00, "Payout" )			PORT_DIPLOCATION("SW1:1")
	PORT_DIPSETTING(    0x00, "Hopper" )
	PORT_DIPSETTING(    0x80, "Manual Payout SW" )
INPUT_PORTS_END

INPUT_PORTS_START( royalcrd )
	PORT_START_TAG("IN0")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_SERVICE ) PORT_NAME("Remote") PORT_CODE(KEYCODE_Q)
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_NAME("Halten (Hold) 1 / Hoch (High)") PORT_CODE(KEYCODE_Z)
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_BUTTON6 ) PORT_NAME("Loeschen/Gamble (Cancel/Play)") PORT_CODE(KEYCODE_N)
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_START1 )  PORT_NAME("Geben (Start)")
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON5 ) PORT_NAME("Halten (Hold) 5 / Half Gamble") PORT_CODE(KEYCODE_B)
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_SERVICE1 ) PORT_NAME("Buchhalt (Service1)")
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_SERVICE2 ) PORT_NAME("Einstellen (Service2)")
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_BUTTON4 ) PORT_NAME("Halten (Hold) 4 / Kassieren (Take)") PORT_CODE(KEYCODE_V)

	PORT_START_TAG("IN1")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_NAME("Halten (Hold) 2") PORT_CODE(KEYCODE_X)
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON3 ) PORT_NAME("Halten (Hold) 3 / Tief (Low)") PORT_CODE(KEYCODE_C)
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_SERVICE ) PORT_NAME("unknown bit 08") PORT_CODE(KEYCODE_E)
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_SERVICE ) PORT_NAME("Hoppersch") PORT_CODE(KEYCODE_W)
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_BUTTON7 ) PORT_NAME("Abschreib (Payout)") PORT_CODE(KEYCODE_M)	// Payout? Need to check with hopper filled.

	PORT_START_TAG("IN2")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START_TAG("DSW")
	PORT_DIPNAME( 0x01, 0x01, "State" )				PORT_DIPLOCATION("SW1:8")
	PORT_DIPSETTING(    0x00, "Keyboard Test" )
	PORT_DIPSETTING(    0x01, "Play" )
	PORT_DIPNAME( 0x02, 0x00, "Remote" )			PORT_DIPLOCATION("SW1:7")	// in some sources is listed as 'Coin-C'
	PORT_DIPSETTING(    0x00, "10 Points/Pulse" )
	PORT_DIPSETTING(    0x02, "50 Points/Pulse" )
	PORT_DIPNAME( 0x04, 0x00, DEF_STR( Coin_B ) )	PORT_DIPLOCATION("SW1:6")
	PORT_DIPSETTING(    0x00, "5 Points/Coin" )
	PORT_DIPSETTING(    0x04, "10 Points/Coin" )
	PORT_DIPNAME( 0x08, 0x00, DEF_STR( Coin_A ) )	PORT_DIPLOCATION("SW1:5")
	PORT_DIPSETTING(    0x00, "5 Points/Coin" )
	PORT_DIPSETTING(    0x08, "10 Points/Coin" )
	PORT_DIPNAME( 0x10, 0x00, "Insert" )			PORT_DIPLOCATION("SW1:4")
	PORT_DIPSETTING(    0x00, "Dattl Insert" )
	PORT_DIPSETTING(    0x10, "TAB Insert" )
	PORT_DIPNAME( 0x20, 0x00, "Joker" )				PORT_DIPLOCATION("SW1:3")
	PORT_DIPSETTING(    0x00, "With Joker" )		// also enable Five of a Kind.
	PORT_DIPSETTING(    0x20, "Without Joker" )
	PORT_DIPNAME( 0x40, 0x00, "Hold" )				PORT_DIPLOCATION("SW1:2")
	PORT_DIPSETTING(    0x00, "Auto Hold" )
	PORT_DIPSETTING(    0x40, "No Auto Hold" )

	/* after nvram init, set the following one to 'manual'
    to allow the remote credits mode to work */

	PORT_DIPNAME( 0x80, 0x00, "Payout" )			PORT_DIPLOCATION("SW1:1")
	PORT_DIPSETTING(    0x00, "Hopper" )
	PORT_DIPSETTING(    0x80, "Manual Payout SW" )
INPUT_PORTS_END

INPUT_PORTS_START( cuoreuno )
	PORT_START_TAG("IN0")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_NAME("Stop 1 / Switch Bet (1-Max)") PORT_CODE(KEYCODE_Z)
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_BUTTON6 ) PORT_NAME("Clear / Bet / Prendi (Take)") PORT_CODE(KEYCODE_N)
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_START1 )  PORT_NAME("Start / Gioca (Play)")
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON5 ) PORT_NAME("Stop 5 / Half Gamble / Super Game") PORT_CODE(KEYCODE_B)
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_SERVICE2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_BUTTON4 ) PORT_NAME("Stop 4 / Alta (High)") PORT_CODE(KEYCODE_V)

	PORT_START_TAG("IN1")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_NAME("Stop 2 / Bassa (Low)") PORT_CODE(KEYCODE_X)
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON3 ) PORT_NAME("Stop 3") PORT_CODE(KEYCODE_C)
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_BUTTON7 ) PORT_NAME("Payout") PORT_CODE(KEYCODE_M)	// Payout? Need to check with hopper filled.

	PORT_START_TAG("IN2")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_UNKNOWN )

	PORT_START_TAG("DSW")
	PORT_DIPNAME( 0x01, 0x01, "Test Mode" )			PORT_DIPLOCATION("SW1:8")
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x02, DEF_STR( Unknown ) )	PORT_DIPLOCATION("SW1:7")
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x04, 0x04, DEF_STR( Unknown ) )	PORT_DIPLOCATION("SW1:6")
	PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Unknown ) )	PORT_DIPLOCATION("SW1:5")
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x10, 0x10, DEF_STR( Unknown ) )	PORT_DIPLOCATION("SW1:4")
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x20, DEF_STR( Unknown ) )	PORT_DIPLOCATION("SW1:3")
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Unknown ) )	PORT_DIPLOCATION("SW1:2")
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	/* the following one (1st DSW) seems to be disconnected
    to avoid the use of remote credits / manual payout */

	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Unused ) )	PORT_DIPLOCATION("SW1:1")
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
INPUT_PORTS_END


/*************************
*    Graphics Layouts    *
*************************/

/*  Magic Card tiles should be 4bpp, but the last two planes
    have flipped tiles, half flipped (bottom half), and lack of others needed.
    (See the text tiles at 0x0840) */

static const gfx_layout charlayout =
{
	4,
	8,
	0x1000,
//  RGN_FRAC(1,2),
	4,
	{ 0, 4, 0x8000*8, 0x8000*8+4 },
//  { RGN_FRAC(0,2), RGN_FRAC(0,2) + 4, RGN_FRAC(1,2), RGN_FRAC(1,2) + 4 },
	{ 3, 2, 1, 0 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 },
	8*4*2
};


/******************************
* Graphics Decode Information *
******************************/

static const gfx_decode gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0x0000, &charlayout, 0, 16 },
	{ -1 }
};


/***********************
*    PIA Interfaces    *
***********************/

static const pia6821_interface pia0_intf =
{
	/* PIA inputs: A, B, CA1, CB1, CA2, CB2 */
	input_port_0_r, input_port_1_r, 0, 0, 0, 0,

	/* PIA outputs: A, B, CA2, CB2 */
	0, 0, 0, 0,

	/* PIA IRQs: A, B */
	0, 0
};

static const pia6821_interface pia1_intf =
{
	/* PIA inputs: A, B, CA1, CB1, CA2, CB2 */
	input_port_2_r, input_port_3_r, 0, 0, 0, 0,

	/* PIA outputs: A, B, CA2, CB2 */
	0, 0, 0, 0,

	/* PIA IRQs: A, B */
	0, 0
};


/************************
*    Sound Interface    *
************************/

static struct AY8910interface ay8910_intf =
{
	0,	// portA in
	0,	// portB in
	funworld_lamp_a_w,	// portA out
	funworld_lamp_b_w	// portB out
};


/**************************
*     Machine Drivers     *
**************************/

static MACHINE_DRIVER_START( funworld )
	// basic machine hardware
	MDRV_CPU_ADD_TAG("main", M65SC02, MASTER_CLOCK/8)	// 2MHz
	MDRV_CPU_PROGRAM_MAP(funworld_map, 0)
	MDRV_CPU_VBLANK_INT(nmi_line_pulse, 1)

	MDRV_SCREEN_REFRESH_RATE(60)
	MDRV_SCREEN_VBLANK_TIME(DEFAULT_60HZ_VBLANK_DURATION)

	MDRV_NVRAM_HANDLER(generic_0fill)

	// video hardware
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
	MDRV_SCREEN_SIZE((124+1)*4, (30+1)*8)		// Taken from MC6845 init, registers 00 & 04. Normally programmed with (value-1).
	MDRV_SCREEN_VISIBLE_AREA(0*4, 96*4-1, 0*8, 29*8-1)	// Taken from MC6845 init, registers 01 & 06.

	MDRV_GFXDECODE(gfxdecodeinfo)

	MDRV_DEFAULT_LAYOUT(layout_funworld)

	MDRV_PALETTE_LENGTH(0x200)
	MDRV_COLORTABLE_LENGTH(0x200)
	MDRV_PALETTE_INIT(funworld)
	MDRV_VIDEO_START(funworld)
	MDRV_VIDEO_UPDATE(funworld)

	// sound hardware
	MDRV_SPEAKER_STANDARD_MONO("mono")

	MDRV_SOUND_ADD_TAG("ay8910", AY8910, MASTER_CLOCK/8)	// 2MHz
	MDRV_SOUND_CONFIG(ay8910_intf)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 8.0)	// analyzed to avoid clips.
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( bigdeal )
	MDRV_IMPORT_FROM(funworld)

	MDRV_SOUND_REPLACE("ay8910", AY8910, MASTER_CLOCK/8)	// 2MHz
	MDRV_SOUND_CONFIG(ay8910_intf)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 5.0)	// analyzed to avoid clips.
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( magiccrd )
	MDRV_IMPORT_FROM(funworld)

	MDRV_CPU_REPLACE("main", M65C02, MASTER_CLOCK/8)	// 2MHz
	MDRV_CPU_PROGRAM_MAP(magiccrd_map, 0)

	MDRV_SCREEN_SIZE((123+1)*4, (36+1)*8)			// Taken from MC6845 init, registers 00 & 04. Normally programmed with (value-1).
	MDRV_SCREEN_VISIBLE_AREA(0*4, 112*4-1, 0*8, 34*8-1)	// Taken from MC6845 init, registers 01 & 06.
//  MDRV_SCREEN_VISIBLE_AREA(0*4, 98*4-1, 0*8, 32*8-1)     // adjusted to screen for testing purposes.

	MDRV_GFXDECODE(gfxdecodeinfo)
	MDRV_VIDEO_START(magiccrd)

	MDRV_SOUND_REPLACE("ay8910", AY8910, MASTER_CLOCK/8)	// 2MHz
	MDRV_SOUND_CONFIG(ay8910_intf)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.6)	// analyzed to avoid clips.
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( royalcrd )
	MDRV_IMPORT_FROM(funworld)

	MDRV_CPU_REPLACE("main", M65C02, MASTER_CLOCK/8)	// original cpu = R65C02P2 (2MHz)
	MDRV_CPU_PROGRAM_MAP(magiccrd_map, 0)
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( cuoreuno )
	MDRV_IMPORT_FROM(funworld)

	MDRV_CPU_REPLACE("main", M65C02, MASTER_CLOCK/8)	// original cpu = R65C02P2 (2MHz)
	MDRV_CPU_PROGRAM_MAP(cuoreuno_map, 0)
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( royalmcu )
	MDRV_IMPORT_FROM(funworld)

	MDRV_CPU_REPLACE("main", M65SC02, MASTER_CLOCK/8)	// original cpu = R65C02P2 (2MHz)
	MDRV_CPU_PROGRAM_MAP(royalmcu_map, 0)
MACHINE_DRIVER_END

/*************************
*        Rom Load        *
*************************/

ROM_START( jollycrd )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )
	ROM_LOAD( "jolycard.run", 0x8000, 0x8000, CRC(f531dd10) SHA1(969191fbfeff4349afef619d9241ef5186e6d57f) )

	ROM_REGION( 0x10000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "jolycard.ch2", 0x0000, 0x8000, CRC(c512b103) SHA1(1f4e78e97855afaf0332fb75e1b5571aafd01c29) )
	ROM_LOAD( "jolycard.ch1", 0x8000, 0x8000, CRC(0f24f39d) SHA1(ac1f6a8a4a2a37cbc0d45c15187b33c25371bffb) )

	ROM_REGION( 0x0200, REGION_PROMS, 0 )
	ROM_LOAD( "82s147.bin",	0x0000, 0x0200, CRC(5ebc5659) SHA1(8d59011a181399682ab6e8ed14f83101e9bfa0c6) )
ROM_END

ROM_START( jolycdae )	// encrypted roms...
	ROM_REGION( 0x18000, REGION_CPU1, 0 )
	ROM_LOAD( "pokeru0.bin", 0x8000, 0x10000, CRC(7732f177) SHA1(b75fca596a7315d1379fa5bcf07c449ec32c90f5) )

	ROM_REGION( 0x20000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "pokeru1.bin", 0x00000, 0x10000, CRC(878d695b) SHA1(0e1ea455e991e591316a340c56b23fa26764988d) )
	ROM_LOAD( "pokeru2.bin", 0x10000, 0x10000, CRC(b7b2946a) SHA1(25b15296b32bf88db6d60991105bba667f7940cc) )
	ROM_COPY( REGION_GFX1,	 0x10000, 0x00000, 0x8000 ) // rgn,srcoffset,offset,length.

	ROM_REGION( 0x0200, REGION_PROMS, 0 )
	ROM_LOAD( "82s147.bin",	0x0000, 0x0200, CRC(5ebc5659) SHA1(8d59011a181399682ab6e8ed14f83101e9bfa0c6) )
ROM_END

ROM_START( jolycdcr )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )
	ROM_LOAD( "jollypkr.003", 0x8000, 0x8000, CRC(ea7340b4) SHA1(7dd468f28a488a4781521809d06db1d7917048ad) )

	ROM_REGION( 0x10000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "jolycard.ch2", 0x0000, 0x8000, CRC(c512b103) SHA1(1f4e78e97855afaf0332fb75e1b5571aafd01c29) )
	ROM_LOAD( "jolycard.ch1", 0x8000, 0x8000, CRC(0f24f39d) SHA1(ac1f6a8a4a2a37cbc0d45c15187b33c25371bffb) )

	ROM_REGION( 0x0200, REGION_PROMS, 0 )
	ROM_LOAD( "82s147.bin",	0x0000, 0x0200, CRC(5ebc5659) SHA1(8d59011a181399682ab6e8ed14f83101e9bfa0c6) )
ROM_END

ROM_START( jolycdit )	// encrypted graphics.
	ROM_REGION( 0x10000, REGION_CPU1, 0 )
	ROM_LOAD( "jn.bin", 0x8000, 0x8000, CRC(6ae00ed0) SHA1(5921c2882aeb5eadd0e04a477fa505ad35e9d98c) )

	ROM_REGION( 0x10000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "2.bin", 0x0000, 0x8000, CRC(46805150) SHA1(63687ac44f6ace6d8924b2629536bcc7d3979ed2) )
	ROM_LOAD( "1.bin", 0x8000, 0x8000, CRC(43bcb2df) SHA1(5022bc3a0b852a7cd433e25c3c90a720e6328261) )

	ROM_REGION( 0x0200, REGION_PROMS, 0 )
	ROM_LOAD( "82s147.bin",	0x0000, 0x0200, CRC(5ebc5659) SHA1(8d59011a181399682ab6e8ed14f83101e9bfa0c6) )

	ROM_REGION( 0x0200, REGION_PLDS, 0 )
	ROM_LOAD( "gal16v8b.bin", 0x0000, 0x0117, CRC(3ad712b1) SHA1(54214841fb178e4b59bf6051522718f7667bad28) )
ROM_END

ROM_START( jolycdat )	// there are unused pieces of code that compare or jumps within $4000-$5000 range.
	ROM_REGION( 0x10000, REGION_CPU1, 0 )
	ROM_LOAD( "bonucard.cpu", 0x8000, 0x4000, CRC(da342100) SHA1(451fa6074aad19e9efd148c3d18115a20a3d344a) )
	ROM_CONTINUE(			  0xc000, 0x4000 )

	ROM_REGION( 0x10000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "jolycard.ch2", 0x0000, 0x8000, CRC(c512b103) SHA1(1f4e78e97855afaf0332fb75e1b5571aafd01c29) )
	ROM_LOAD( "jolycard.ch1", 0x8000, 0x8000, CRC(0f24f39d) SHA1(ac1f6a8a4a2a37cbc0d45c15187b33c25371bffb) )

	ROM_REGION( 0x0200, REGION_PROMS, 0 )
	ROM_LOAD( "82s147.bin",	0x0000, 0x0200, CRC(5ebc5659) SHA1(8d59011a181399682ab6e8ed14f83101e9bfa0c6) )	// jollycrd palette till a dump appear.
ROM_END

ROM_START( jolycdab )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )	// program is testing/writting RAM in offset $8800-$BFFF (ROM)...??
	ROM_LOAD( "ig1poker.run", 0x8000, 0x8000, CRC(c96e6542) SHA1(ed6c0cf9fe8597dba9149b2225320d8d9c39219a) )
//  ROM_RELOAD(               0x4000, 0x4000 )

	ROM_REGION( 0x10000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "jn1poker.ch2", 0x0000, 0x8000, CRC(8d78e43d) SHA1(15c60f8e0cd88518b0dc72b92aff6d8d4b2149cf) )
	ROM_LOAD( "jn1poker.ch1", 0x8000, 0x8000, CRC(d0a87f58) SHA1(6b7925557c4e40a1ebe52ecd14391cdd5e00b59a) )

	ROM_REGION( 0x0200, REGION_PROMS, 0 )
	ROM_LOAD( "82s147.bin",	0x0000, 0x0200, CRC(5ebc5659) SHA1(8d59011a181399682ab6e8ed14f83101e9bfa0c6) )
ROM_END

ROM_START( bigdeal )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )
	ROM_LOAD( "poker4.001", 0x8000, 0x8000, CRC(bb0198c1) SHA1(6e7d42beb5723a4368ae3788f83b448f86e5653d) )

	ROM_REGION( 0x10000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "poker4.003", 0x0000, 0x8000, CRC(8c33a15f) SHA1(a1c8451c99a23eeffaedb21d1a1b69f54629f8ab) )
	ROM_LOAD( "poker4.002", 0x8000, 0x8000, CRC(5f4e12d8) SHA1(014b2364879faaf4922cdb82ee07692389f20c2d) )

	ROM_REGION( 0x0200, REGION_PROMS, 0 )
	ROM_LOAD( "82s147.bin",	0x0000, 0x0200, CRC(5ebc5659) SHA1(8d59011a181399682ab6e8ed14f83101e9bfa0c6) )	// jollycrd palette till a dump appear.
ROM_END

ROM_START( bigdealb )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )
	ROM_LOAD( "poker8.003", 0x8000, 0x8000, CRC(09c93745) SHA1(a64e96ef3489bc37c2c642f49e62cfef371de6f1) )

	ROM_REGION( 0x10000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "poker4.003", 0x0000, 0x8000, CRC(8c33a15f) SHA1(a1c8451c99a23eeffaedb21d1a1b69f54629f8ab) )
	ROM_LOAD( "poker4.002", 0x8000, 0x8000, CRC(5f4e12d8) SHA1(014b2364879faaf4922cdb82ee07692389f20c2d) )

	ROM_REGION( 0x0200, REGION_PROMS, 0 )
	ROM_LOAD( "82s147.bin",	0x0000, 0x0200, CRC(5ebc5659) SHA1(8d59011a181399682ab6e8ed14f83101e9bfa0c6) )	// jollycrd palette till a dump appear.
ROM_END

ROM_START( cuoreuno )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )
	ROM_LOAD( "cuore1a.u2", 0x8000, 0x8000, CRC(6e112184) SHA1(283ac534fc1cb33d11bbdf3447630333f2fc957f) )

	ROM_REGION( 0x10000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "cuore1b.u21", 0x0000, 0x8000, CRC(14eca2b8) SHA1(35cba415800c6cd3e6ed9946057f33510ad2bfc9) )
	ROM_LOAD( "cuore1c.u22", 0x8000, 0x8000, CRC(253fac84) SHA1(1ad104ab8e8d73df6397a840a4b26565b245d7a3) )

	ROM_REGION( 0x0200, REGION_PROMS, 0 )
	ROM_LOAD( "am27s29_cu.bin",    0x0000, 0x0200, CRC(7ea61749) SHA1(3167acd79f9bda2078c2af3e049ad6abf160aeae) )

	ROM_REGION( 0x0600, REGION_PLDS, 0 )
	ROM_LOAD( "palce16v8h_cu.u5",  0x0000, 0x0117, NO_DUMP ) /* PAL is read protected */
	ROM_LOAD( "palce20v8h_cu.u22", 0x0200, 0x0157, NO_DUMP ) /* PAL is read protected */
	ROM_LOAD( "palce20v8h_cu.u23", 0x0400, 0x0157, NO_DUMP ) /* PAL is read protected */
ROM_END

ROM_START( elephfam )
	ROM_REGION( 0x18000, REGION_CPU1, 0 )
	ROM_LOAD( "eleph_a.u2", 0x8000, 0x10000, CRC(8392b842) SHA1(74c850c734ca8174167b2f826b9b1ac902669392) )

	ROM_REGION( 0x20000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "eleph_c.u22", 0x00000, 0x10000, CRC(4b909bf3) SHA1(a822b12126bc58af6d3f999ab2117370015a039b) )
	ROM_LOAD( "eleph_b.u21", 0x10000, 0x10000, CRC(e3612670) SHA1(beb65f7d2bd6d7bc68cfd876af51910cf6417bd0) )
	ROM_COPY( REGION_GFX1,	 0x10000, 0x00000, 0x8000 ) // rgn,srcoffset,offset,length.

	ROM_REGION( 0x0200, REGION_PROMS, 0 )
	ROM_LOAD( "am27s29_ef.u25",    0x0000, 0x0200, CRC(bca8b82a) SHA1(4aa19f5ecd9953bf8792dceb075a746f77c01cfc) )

	ROM_REGION( 0x0600, REGION_PLDS, 0 )
	ROM_LOAD( "palce16v8h_ef.u5",  0x0000, 0x0117, NO_DUMP ) /* PAL is read protected */
	ROM_LOAD( "palce20v8h_ef.u22", 0x0200, 0x0157, NO_DUMP ) /* PAL is read protected */
	ROM_LOAD( "palce20v8h_ef.u23", 0x0400, 0x0157, NO_DUMP ) /* PAL is read protected */
ROM_END

ROM_START( elephfmb )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )
	ROM_LOAD( "now.u2", 0x8000, 0x8000, CRC(7b537ce6) SHA1(b221d08c53b9e14178335632420e78070b9cfb27) )

	ROM_REGION( 0x10000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "elephb.u21", 0x0000, 0x8000, CRC(3c60549c) SHA1(c839b3ea415a877e5eac04e0522c342cce8d6e64) )
	ROM_LOAD( "elephc.u20", 0x8000, 0x8000, CRC(448ba955) SHA1(2785cbc8cd42a7dda85bd8b81d5fbec01a1ba0bd) )

	ROM_REGION( 0x0200, REGION_PROMS, 0 )
	ROM_LOAD( "am27s29_ef.u25",    0x0000, 0x0200, CRC(bca8b82a) SHA1(4aa19f5ecd9953bf8792dceb075a746f77c01cfc) )

	ROM_REGION( 0x0600, REGION_PLDS, 0 )
	ROM_LOAD( "palce16v8h_ef.u5",  0x0000, 0x0117, NO_DUMP ) /* PAL is read protected */
	ROM_LOAD( "palce20v8h_ef.u22", 0x0200, 0x0157, NO_DUMP ) /* PAL is read protected */
	ROM_LOAD( "palce20v8h_ef.u23", 0x0400, 0x0157, NO_DUMP ) /* PAL is read protected */
ROM_END

ROM_START( pool10 )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )
	ROM_LOAD( "pool10.u2", 0x8000, 0x8000, CRC(4e928756) SHA1(d9ac3d41ea32e060a7e269502b7f22333c5e6c61) )

	ROM_REGION( 0x10000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "2.u21", 0x0000, 0x8000, CRC(99c8c074) SHA1(f8082b08e895cbcd028a2b7cd961a7a2c8b2762c) )
	ROM_LOAD( "1.u20", 0x8000, 0x8000, CRC(9abedd0c) SHA1(f184a82e8ec2387069d631bcb77e890acd44b3f5) )

	ROM_REGION( 0x0200, REGION_PROMS, 0 )
	ROM_LOAD( "n82s147an_p10.u25",    0x0000, 0x0200, CRC(1de03d14) SHA1(d8eda20865c1d885a428931f4380032e103b252c) )

	ROM_REGION( 0x0600, REGION_PLDS, 0 )
	ROM_LOAD( "palce16v8h_p10.u5",  0x0000, 0x0117, NO_DUMP ) /* PAL is read protected */
	ROM_LOAD( "gal20v8b_p10.u22", 0x0200, 0x0157, NO_DUMP ) /* GAL is read protected */
	ROM_LOAD( "gal20v8b_p10.u23", 0x0400, 0x0157, NO_DUMP ) /* GAL is read protected */
ROM_END

ROM_START( pool10b )
	ROM_REGION( 0x18000, REGION_CPU1, 0 )
	ROM_LOAD( "u2.bin", 0x8000, 0x10000, CRC(64fee38e) SHA1(8a624a0b6eb4a3ba09e5b396dc5a01994dfdf294) )

	/* GFX ROMs are the same of pool10, but double sized with identical halves. */
	ROM_REGION( 0x20000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "u20.bin", 0x00000, 0x10000, CRC(3bdf1106) SHA1(fa21cbd49bb27ea4a784cf4e4b3fbd52650a285b) )
	ROM_LOAD( "u21.bin", 0x10000, 0x10000, CRC(581c4878) SHA1(5ae61af090feea1745e22f46b33b2c01e6013fbe) )
	ROM_COPY( REGION_GFX1,	 0x10000, 0x00000, 0x8000 ) // rgn,srcoffset,offset,length.

	ROM_REGION( 0x0200, REGION_PROMS, 0 )
	ROM_LOAD( "n82s147an_p10.u25",    0x0000, 0x0200, CRC(1de03d14) SHA1(d8eda20865c1d885a428931f4380032e103b252c) )

	ROM_REGION( 0x0600, REGION_PLDS, 0 )
	ROM_LOAD( "palce16v8h_p10b.u5",  0x0000, 0x0117, NO_DUMP ) /* PAL is read protected */
	ROM_LOAD( "palce20v8h_p10b.u22", 0x0200, 0x0157, NO_DUMP ) /* PAL is read protected */
	ROM_LOAD( "palce20v8h_p10b.u23", 0x0400, 0x0157, NO_DUMP ) /* PAL is read protected */
ROM_END

ROM_START( tortufam )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )
	ROM_LOAD( "tortu.a.u2", 0x8000, 0x8000, CRC(6e112184) SHA1(283ac534fc1cb33d11bbdf3447630333f2fc957f) )

	ROM_REGION( 0x10000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "tortu.b.u21", 0x0000, 0x8000, CRC(e7b18584) SHA1(fa1c367469d4ced5d7c83c15a25ec5fd6afcca10) )
	ROM_LOAD( "tortu.c.u20", 0x8000, 0x8000, CRC(3cda6f73) SHA1(b4f3d2d3c652ebf6973358ae33b7808de5939acd) )

	ROM_REGION( 0x0200, REGION_PROMS, 0 )
	ROM_LOAD( "am27s29pc_tf.u25",    0x0000, 0x0200, CRC(c6d433fb) SHA1(065de832bbe8765eb0aacc2029e587a4f5362f8a) )

	ROM_REGION( 0x0600, REGION_PLDS, 0 )
	ROM_LOAD( "palce20v8h_tf.u5",  0x0000, 0x0157, NO_DUMP ) /* PAL is read protected */
	ROM_LOAD( "palce20v8h_tf.u22", 0x0200, 0x0157, NO_DUMP ) /* PAL is read protected */
	ROM_LOAD( "palce20v8h_tf.u23", 0x0400, 0x0157, NO_DUMP ) /* PAL is read protected */
ROM_END

ROM_START( royalcrd )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )
	ROM_LOAD( "r2.bin", 0x8000, 0x8000, CRC(25dfe0dc) SHA1(1a857a910d0c34b6b5bfc2b6ea2e08ed8ed0cae0) )

	ROM_REGION( 0x10000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "2.bin", 0x0000, 0x8000, CRC(85e77661) SHA1(7d7a765c1bfcfeb9eb91d2519b22d734f20eab24) )
	ROM_LOAD( "1.bin", 0x8000, 0x8000, CRC(41f7a0b3) SHA1(9aff2b8832d2a4f868daa9849a0bfe5e44f88fc0) )

	ROM_REGION( 0x0200, REGION_PROMS, 0 )
	ROM_LOAD( "n82s147.bin",    0x0000, 0x0200, CRC(8bc86f48) SHA1(4c677ab9314a1f571e35104b22659e6811aeb194) )

	ROM_REGION( 0x0600, REGION_PLDS, 0 )
	ROM_LOAD( "palce16v8h-4.bin",  0x0000, 0x0117, NO_DUMP ) /* PAL is read protected */
	ROM_LOAD( "1-peel18cv8.bin",   0x0200, 0x0155, NO_DUMP ) /* PEEL is read protected */
	ROM_LOAD( "2-peel18cv8.bin",   0x0400, 0x0155, CRC(8fdafd55) SHA1(fbb187ba682111648ea1586f400990cb81a3077a) )
ROM_END

ROM_START( royalcdb ) // both halves have different program. we're using the 2nd one.
	ROM_REGION( 0x20000, REGION_CPU1, 0 )	// 1st half prg is testing RAM in offset $8600-$BF00...??
	ROM_LOAD( "rc.bin", 0x10000, 0x10000, CRC(8a9a6dd6) SHA1(04c3f9f17d5404ac1414c51ef8f930df54530e72) )
	ROM_COPY( REGION_CPU1,	0x18000, 0x8000, 0x8000 )

	ROM_REGION( 0x20000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "1a.bin", 0x0000, 0x10000, CRC(8a66f22c) SHA1(67d6e8f8f5a0fd979dc498ba2cc67cf707ccdf95) )
	ROM_LOAD( "2a.bin", 0x10000, 0x10000, CRC(3af71cf8) SHA1(3a0ce0d0abebf386573c5936545dada1d3558e55) )
	ROM_COPY( REGION_GFX1,	 0x10000, 0x00000, 0x8000 ) // rgn,srcoffset,offset,length.

	ROM_REGION( 0x0200, REGION_PROMS, 0 )
	ROM_LOAD( "n82s147.bin",    0x0000, 0x0200, CRC(8bc86f48) SHA1(4c677ab9314a1f571e35104b22659e6811aeb194) )

	ROM_REGION( 0x0200, REGION_PLDS, 0 )
	ROM_LOAD( "palce16v8h-4.bin",   0x0000, 0x0117, NO_DUMP ) /* PAL is read protected */
ROM_END

ROM_START( royalcdc )	// encrypted program rom.
	ROM_REGION( 0x10000, REGION_CPU1, 0 )
	ROM_LOAD( "rc_1.bin", 0x8000, 0x8000, CRC(8cdcc978) SHA1(489b58760a7c8646399c8cdfb86ec4341823e7dd) )

	ROM_REGION( 0x10000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "rc_3.bin", 0x0000, 0x8000, CRC(8612c6ed) SHA1(3306a252af479e0510f136020086015b60dce879) )
	ROM_LOAD( "rc_2.bin", 0x8000, 0x8000, CRC(7f934488) SHA1(c537a09ef7e88a81ee9c2e1d971b3caf9d3dba0e) )

	ROM_REGION( 0x0200, REGION_PROMS, 0 )
	ROM_LOAD( "82s147.bin",    0x0000, 0x0200, CRC(44dbf086) SHA1(43a2d615c00605db75a4fd4d57d9e056c0356f10) )

	ROM_REGION( 0x0600, REGION_PLDS, 0 )
	ROM_LOAD( "palce16v8.bin",  0x0000, 0x0117, NO_DUMP )	// not present in the set.
	ROM_LOAD( "1-peel18cv8p.bin",   0x0200, 0x0155, NO_DUMP )	// not present in the set.
	ROM_LOAD( "2-peel18cv8p.bin",   0x0400, 0x0155, NO_DUMP )	// not present in the set.
ROM_END

ROM_START( magiccrd )
	ROM_REGION( 0x18000, REGION_CPU1, 0 )
    ROM_LOAD( "magicard.004", 0x0000, 0x8000,  CRC(f6e948b8) SHA1(7d5983015a508ab135ccbf69b7f3c526c229e3ef) ) // only last 16kbyte visible?
	ROM_LOAD( "magicard.01",  0x8000, 0x10000, CRC(c94767d4) SHA1(171ac946bdf2575f9e4a31e534a8e641597af519) ) // 1ST AND 2ND HALF IDENTICAL

	ROM_REGION( 0x10000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "magicard.03",  0x0000, 0x8000, CRC(733da697) SHA1(45122c64d5a371ec91cecc67b7faf179078e714d) )
	ROM_LOAD( "magicard.001", 0x8000, 0x8000, CRC(685f7a67) SHA1(fc65a9d26a5cbbe2fa08dc0f27212dae2d8bcbdc) ) // bad dump?, or sprite plane

	ROM_REGION( 0x0200, REGION_PROMS, 0 )
	ROM_LOAD( "82s147_mc.bin",	0x0000, 0x0200, NO_DUMP )	// not present in the set.
ROM_END

ROM_START( jokercrd )
/*  (Multi) Joker Card from Vesely Svet (Sprightly World). Czech poker game.
    Program roms seems encrypted. Custom Fun World CPU based on 6502 family.
    Seems to be a Big Deal clone.
*/
	ROM_REGION( 0x18000, REGION_CPU1, 0 )
    ROM_LOAD( "ic41.bin",   0x8000,  0x8000, CRC(d36188b3) SHA1(3fb848fabbbde9fbb70875b3dfef62bfb3a8cbcb) ) // only last 16kbyte visible?
	ROM_LOAD( "ic37.bin",   0x10000, 0x8000, CRC(8e0d70c4) SHA1(018f92631acbe98e5826a41698f0e07b4b46cd71) ) // 1ST AND 2ND HALF IDENTICAL
	ROM_COPY( REGION_CPU1,	0x10000, 0xc000, 0x4000 ) // rgn,srcoffset,offset,length.

	ROM_REGION( 0x10000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "ic10.bin",	0x0000, 0x8000, CRC(2bbd27ad) SHA1(37d37899398d95beac5f3cbffc4277c97aca1a23) )
	ROM_LOAD( "ic11.bin",	0x8000, 0x8000, CRC(21d05a57) SHA1(156c18ec31b08e4c4af6f73b49cb5d5c68d1670f) ) // bad dump?, or sprite plane

	ROM_REGION( 0x0200, REGION_PROMS, 0 )
	ROM_LOAD( "ic13.bin",	0x0000, 0x0200, CRC(e59fc06e) SHA1(88a3bb89f020fe2b20f768ca010a082e0b974831) )
ROM_END

ROM_START( monglfir )
	ROM_REGION( 0x20000, REGION_CPU1, 0 )
	ROM_LOAD( "prgteov.2.3m.u16", 0x10000, 0x10000, CRC(996b851a) SHA1(ef4e3d036ca10b33c83749024d04c4d4c09feeb7) )
	ROM_COPY( REGION_CPU1,	0x18000, 0x8000, 0x8000 )

	ROM_REGION( 0x10000, REGION_CPU2, 0 ) /* TSC87C52-16CB MCU Code */
	ROM_LOAD( "tsc87c52-mf.u40", 0x00000, 0x02000 , CRC(ae22e778) SHA1(0897e05967d68d7f23489e98717663e3a3176070) )

	ROM_REGION( 0x20000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "mong.rc.b2.u3", 0x00000, 0x10000, CRC(5e019b73) SHA1(63a544dccb9589e5a6b938e604c09d4d8fc060fc) )
	ROM_LOAD( "mong.rc.c1.u2", 0x10000, 0x10000, CRC(e3fc24c4) SHA1(ea4e67ace63b55a76365f7e11a67c7d420a52dd7) )
	ROM_COPY( REGION_GFX1,	 0x10000, 0x8000, 0x8000 ) // rgn,srcoffset,offset,length.

	ROM_REGION( 0x0200, REGION_PROMS, 0 )
	ROM_LOAD( "am27s29pc_mf.u24",    0x0000, 0x0200, CRC(da9181af) SHA1(1b30d992f3b2a4b3bd81e3f99632311988e2e8d1) )

	ROM_REGION( 0x0200, REGION_PLDS, 0 )
	ROM_LOAD( "palce16v8h_mf.u5",  0x0000, 0x0117, NO_DUMP ) /* PAL is read protected */
ROM_END

ROM_START( soccernw )
	ROM_REGION( 0x20000, REGION_CPU1, 0 )
	ROM_LOAD( "prgteo2gv2.3.u16", 0x10000, 0x10000, CRC(c61d1937) SHA1(c516f13a108da60b7ccee338b63a025009ef9099) )
	ROM_COPY( REGION_CPU1,	0x18000, 0x8000, 0x8000 )

	ROM_REGION( 0x10000, REGION_CPU2, 0 ) /* TSC87C52-16CB MCU Code */
	ROM_LOAD( "tsc87c52-sn.u40", 0x00000, 0x02000 , CRC(af0bd35b) SHA1(c6613a7bcdec2fd6060d6dcf639654568de87e75) )

	ROM_REGION( 0x20000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "soccer2.u3", 0x00000, 0x10000, CRC(db09b5bb) SHA1(a12bf2938f5482ea5ebc0db6fd6594e1beb97017) )
	ROM_LOAD( "soccer1.u2", 0x10000, 0x10000, CRC(564cc467) SHA1(8f90c4bacd97484623666b25dae77e628908e243) )
	ROM_COPY( REGION_GFX1,	 0x10000, 0x8000, 0x8000 ) // rgn,srcoffset,offset,length.

	ROM_REGION( 0x0200, REGION_PROMS, 0 )
	ROM_LOAD( "am27s29pc_sn.u24",    0x0000, 0x0200, CRC(d02894fc) SHA1(adcdc912cc0b7a7f67b122fa94fca921c957b282) )

	ROM_REGION( 0x0200, REGION_PLDS, 0 )
	ROM_LOAD( "palce16v8h_sn.u5",  0x0000, 0x0117, NO_DUMP ) /* PAL is read protected */
ROM_END


/**************************
*  Driver Initialization  *
**************************/

static DRIVER_INIT( funworld )
{
	/* Initializing PIAs... */
	pia_config(0, PIA_STANDARD_ORDERING, &pia0_intf);
	pia_config(1, PIA_STANDARD_ORDERING, &pia1_intf);
}

static DRIVER_INIT( jolycdit )
{
/*  The encryption seems to be...

    For each nibble:

    xe = (3 * xd) & 0f  ; where xe = encrypted value, xd = decrypted value.

    and then 3 swapped pairs... (3,7) (b,f) (6,e)

    ...or cases (3,7,b,f) ^ 4
    ...and cases (6,e) ^ 8

    to get the same result.

    Since the table is very short, we will substitute each nibble.
*/

	int x, na, nb, nad, nbd;
	UINT8 *src = memory_region( REGION_GFX1 );

	for (x=0x0000;x<0x10000;x++)
	{
		na = src[x] & 0xf0;	// nibble a
		nb = src[x] & 0x0f;	// nibble b

		switch (na)
		{
			case 0x10: nad = 0x30; break;
			case 0x20: nad = 0x60; break;
			case 0x30: nad = 0x50; break;
			case 0x40: nad = 0xc0; break;
			case 0x50: nad = 0xf0; break;
			case 0x60: nad = 0xa0; break;
			case 0x70: nad = 0x90; break;
			case 0x90: nad = 0xb0; break;
			case 0xa0: nad = 0xe0; break;
			case 0xb0: nad = 0xd0; break;
			case 0xc0: nad = 0x40; break;
			case 0xd0: nad = 0x70; break;
			case 0xe0: nad = 0x20; break;
			case 0xf0: nad = 0x10; break;
			default: nad = na; break;
		}

		switch (nb)
		{
			case 0x01: nbd = 0x03; break;
			case 0x02: nbd = 0x06; break;
			case 0x03: nbd = 0x05; break;
			case 0x04: nbd = 0x0c; break;
			case 0x05: nbd = 0x0f; break;
			case 0x06: nbd = 0x0a; break;
			case 0x07: nbd = 0x09; break;
			case 0x09: nbd = 0x0b; break;
			case 0x0a: nbd = 0x0e; break;
			case 0x0b: nbd = 0x0d; break;
			case 0x0c: nbd = 0x04; break;
			case 0x0d: nbd = 0x07; break;
			case 0x0e: nbd = 0x02; break;
			case 0x0f: nbd = 0x01; break;
			default: nbd = nb; break;
		}

		src[x] = nad + nbd;	// decrypted value.
	}

	/* Initializing PIAs... */
	pia_config(0, PIA_STANDARD_ORDERING, &pia0_intf);
	pia_config(1, PIA_STANDARD_ORDERING, &pia1_intf);
}

static DRIVER_INIT( jolycdae )
{
	/* decrypting roms... */

	int x;
	UINT8 *srcp = memory_region( REGION_CPU1 );
	UINT8 *srcg = memory_region( REGION_GFX1 );

	for (x=0x8000;x<0x18000;x++)
	{
		srcp[x] = srcp[x] ^ 0x56;	// simple XOR with 0x56
	}

	for (x=0x0000;x<0x20000;x++)
	{
		srcg[x] = srcg[x] ^ 0x56;	// simple XOR with 0x56
	}

	/* Initializing PIAs... */
	pia_config(0, PIA_STANDARD_ORDERING, &pia0_intf);
	pia_config(1, PIA_STANDARD_ORDERING, &pia1_intf);
}

static DRIVER_INIT( soccernw )
{
/* temporary patch to avoid hardware errors for debug purposes */
	UINT8 *ROM = memory_region(REGION_CPU1);

	ROM[0x80b2] = 0xa9;
	ROM[0x80b3] = 0x00;

//  DEBUG
//  run to $810a

//  ROM[0xa33a] = 0xea;
//  ROM[0xa33b] = 0xea;
//  ROM[0xa33c] = 0xea;

	/* Initializing PIAs... */
	pia_config(0, PIA_STANDARD_ORDERING, &pia0_intf);
	pia_config(1, PIA_STANDARD_ORDERING, &pia1_intf);
}


/*************************
*      Game Drivers      *
*************************/

//    YEAR  NAME      PARENT    MACHINE   INPUT     INIT            COMPANY            FULLNAME
GAME( 1985, jollycrd, 0,        funworld, funworld, funworld, ROT0, "TAB-Austria",     "Jolly Card (Austria)", 0 )
GAME( 1985, jolycdae, jollycrd, funworld, funworld, jolycdae, ROT0, "TAB-Austria",     "Jolly Card (Austria, encrypted)", 0 )
GAME( 1993, jolycdcr, jollycrd, cuoreuno, jolycdcr, funworld, ROT0, "Soft Design",     "Jolly Card (Croatia)", 0 )
GAME( 199?, jolycdit, jollycrd, cuoreuno, jolycdit, jolycdit, ROT0, "bootleg",         "Jolly Card (Italia, encrypted)", 0 )
GAME( 1990, jolycdab, jollycrd, funworld, funworld, funworld, ROT0, "Inter Games",     "Jolly Card (Austria, Fun World, bootleg)", GAME_NOT_WORKING )
GAME( 1986, bigdeal,  0,        bigdeal,  bigdeal,  funworld, ROT0, "Fun World",       "Big Deal (Hungary, set 1)", GAME_IMPERFECT_COLORS )
GAME( 1986, bigdealb, bigdeal,  bigdeal,  bigdeal,  funworld, ROT0, "Fun World",       "Big Deal (Hungary, set 2)", GAME_IMPERFECT_COLORS )
GAME( 1986, jolycdat, bigdeal,  bigdeal,  bigdeal,  funworld, ROT0, "Fun World",       "Jolly Card (Austria, Fun World)", GAME_IMPERFECT_COLORS )
GAME( 1996, cuoreuno, 0,        cuoreuno, cuoreuno, funworld, ROT0, "C.M.C.",          "Cuore 1 (Italia)", 0 )
GAME( 1997, elephfam, 0,        cuoreuno, cuoreuno, funworld, ROT0, "C.M.C.",          "Elephant Family (Italia, new)", 0 )
GAME( 1996, elephfmb, elephfam, cuoreuno, cuoreuno, funworld, ROT0, "C.M.C.",          "Elephant Family (Italia, old)", 0 )
GAME( 1996, pool10,   0,        cuoreuno, cuoreuno, funworld, ROT0, "C.M.C.",          "Pool 10 (Italia, set 1)", 0 )
GAME( 1996, pool10b,  pool10,   cuoreuno, cuoreuno, funworld, ROT0, "C.M.C.",          "Pool 10 (Italia, set 2)", 0 )
GAME( 1997, tortufam, 0,        cuoreuno, cuoreuno, funworld, ROT0, "C.M.C.",          "Tortuga Family (Italia)", 0 )
GAME( 1991, royalcrd, 0,        royalcrd, royalcrd, funworld, ROT0, "TAB-Austria",     "Royal Card (Austria, set 1)", 0 )
GAME( 1991, royalcdb, royalcrd, royalcrd, royalcrd, funworld, ROT0, "TAB-Austria",     "Royal Card (Austria, set 2)", 0 )
GAME( 1991, royalcdc, royalcrd, royalcrd, royalcrd, funworld, ROT0, "Evona Electronic","Royal Card (Slovakia, encrypted)", GAME_WRONG_COLORS | GAME_NOT_WORKING )
GAME( 1996, magiccrd, 0,        magiccrd, magiccrd, funworld, ROT0, "Impera",          "Magic Card II (Bulgaria, bootleg)", GAME_WRONG_COLORS | GAME_IMPERFECT_GRAPHICS | GAME_IMPERFECT_SOUND )
GAME( 1993, jokercrd, 0,        funworld, funworld, funworld, ROT0, "Vesely Svet",     "Joker Card (Ver.A267BC, encrypted)", GAME_WRONG_COLORS | GAME_NOT_WORKING )
GAME( 199?, monglfir, 0,        royalmcu, royalcrd, funworld, ROT0, "bootleg",         "Mongolfier New (Italia)", GAME_UNEMULATED_PROTECTION | GAME_NOT_WORKING )
GAME( 199?, soccernw, 0,        royalcrd, royalcrd, soccernw, ROT0, "bootleg",         "Soccer New (Italia)", GAME_UNEMULATED_PROTECTION | GAME_NOT_WORKING )
