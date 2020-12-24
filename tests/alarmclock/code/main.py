'''
Project: Kail Birthday Present
Author(s): Pekka Lehtikoski and Sofie Lehtikoski
Description: An expandable life-manager device, starting 
             with an alarm clock and inspirational message 
             selector. :)
'''
import time
import datetime
import RPi.GPIO as GPIO
import threading
from pygame import mixer
import math
import copy
import os

#import st7735s as controller
#import Python_ST7735.ST7735 as TFT
#import Adafruit_GPIO.SPI as SPI
#from st7735_tft.st7735_tft import ST7735_TFT
from st7735.library.ST7735 import ST7735 as ST7735_TFT

from PIL import Image

GPIO.setmode(GPIO.BOARD) 

# HARDWARE COMPONENTS AND ABSTRACTION
class ButtonController(object):
    ''' 
    The base controller class for each of the buttons. 
    Allows support for interfacing with the display and 
    speaker.
    '''

    def __init__(self,):
        pass

class InspirationalButton(ButtonController):
    ''' 
    Button that shuffles through inspirational 
    messages and outputs them to the speaker
    '''
    def __init__(self,speaker,channel):
        self.speaker = speaker

        # Start editing mode button
        midChnl = channel
        GPIO.setup(midChnl, GPIO.IN, pull_up_down=GPIO.PUD_DOWN) 
        GPIO.add_event_detect(midChnl, GPIO.RISING) 
        GPIO.add_event_callback(midChnl, self.speaker.playInspirationMsg) 

class SetAlarmButton(ButtonController):
    '''
    Button that sets the alarm time on the display and
    internal program. Interfaces with the display and directly
    feeds it instructions based on the editing button feedback.
    '''
    def __init__(self,display,channels):
        # Initialize display
        #channels format: {'MID':10,'UP':29,'DOWN':31,'LEFT':7,'RIGHT':15}
        self.display = display

        # SET UP GPIO PINs for the given button
        #
        # Start editing mode button
        midChnl = channels['MID'] # SET button
        GPIO.setup(midChnl, GPIO.IN, pull_up_down=GPIO.PUD_DOWN) 
        GPIO.add_event_detect(midChnl, GPIO.BOTH) 
        GPIO.add_event_callback(midChnl, self.display.startEditingMode)

        # Direction buttons
        midChnl = channels['UP'] 
        GPIO.setup(midChnl, GPIO.IN, pull_up_down=GPIO.PUD_DOWN) 
        GPIO.add_event_detect(midChnl, GPIO.BOTH)
        GPIO.add_event_callback(midChnl, self.display.incrementDigit)

        midChnl = channels['DOWN'] 
        GPIO.setup(midChnl, GPIO.IN, pull_up_down=GPIO.PUD_DOWN) 
        GPIO.add_event_detect(midChnl, GPIO.BOTH) 
        GPIO.add_event_callback(midChnl, self.display.decrementDigit)

        midChnl = channels['LEFT'] 
        GPIO.setup(midChnl, GPIO.IN, pull_up_down=GPIO.PUD_DOWN) 
        GPIO.add_event_detect(midChnl, GPIO.BOTH) 
        GPIO.add_event_callback(midChnl, self.display.selectLeftDigit)

        midChnl = channels['RIGHT'] 
        GPIO.setup(midChnl, GPIO.IN, pull_up_down=GPIO.PUD_DOWN) 
        GPIO.add_event_detect(midChnl, GPIO.BOTH) 
        GPIO.add_event_callback(midChnl, self.display.selectRightDigit)

        #if GPIO.input(10):
        #GPIO.wait_for_edge(10, GPIO.RISING)

    def getAlarmTime(self,):
        # Gets the stored alarm time displayed
        return self.display.getAlarmTime()
    
class OnOffSwitch(ButtonController):
    '''
    TOGGLE SWITCH that is able to turn the alarm on and off.
    Does not need to interface with any other hardware,
    only the base program internals.
    '''
    #isOn = True # TODO: Delete when GpIO is actually connected
    def __init__(self,channel):
        ''' Sets the hardware switch based on the on/off '''
        self.chnnl = channel
        GPIO.setup(self.chnnl, GPIO.IN, pull_up_down=GPIO.PUD_DOWN)

    def checkIsOn(self,):
        ''' 
        If hardware switch is ON (connects wires), then we should 
        return False (reversed logic so that when the switch is
        entirely disconnected, it by default returns True) 
        '''
        #print('GPIO input')
        #print(GPIO.input(self.chnnl))
        return not GPIO.input(self.chnnl)

class Speaker(object):
    ''' 
    A wrapper for the speaker hardware to allow basic controls 
    in Python. Includes controls for grabbing the audioclip to play.

    Future dreams:
    - connect speaker via bluetooth insteadof GPIO?
    - connect speaker to webradio for a radio or music option
    '''
    basepath = "/coderoot/pins/tests/alarmclock/code"# '/'.join(__file__.split('/')[:-1])  # TODO: Hardcoded path
    audiopath = basepath + '/audiofiles'
    print(audiopath)
    inspirational_msgs = [
        # List of inspirational messages, as audio files to pull from
        './audiofiles/inspiration/'+filename for filename in os.listdir(audiopath+'/inspiration')
    ] 
    morning_msgs       = [
        # List of wake-up messages played at the alarm times (also audio files)
        './audiofiles/morning/'+filename for filename in os.listdir(audiopath+'/morning')
    ] 

    def __init__(self,channels):
        ''' Sets up the Speaker pi channel '''
        # Raspberry Pi GPIO stuff
        #self.leftChannel = channels["LEFT"] # Pins will be configured in 
                                            # operating system for auto-detection, not here
        #GPIO.setup(self.leftChannel, GPIO.OUT, initial=GPIO.LOW)

        # Sound play-back setup
        mixer.init(buffer=2048) # NOTE: Small buffer size allocated to prevent memory/underrun issues with the pi 0

    def messageShuffle(self, messageList):
        ''' 
        Selects a message from the messageList 
        at random and outputs it 
        '''
        import random
        ind = random.randint(0,len(messageList)-1)
        return messageList[ind]

    def playMorningAlarm(self,):
        ''' Plays a random morning alarm from the morning audio set '''
        audiofile = self.messageShuffle(self.morning_msgs)
        self.startSound(audiofile)

    def playInspirationMsg(self,callback_channel):
        ''' Plays inspirational message from the inspirational audio set '''
        audiofile = self.messageShuffle(self.inspirational_msgs)
        self.startSound(audiofile)

    def startSound(self,soundclip):
        ''' 
        Starts playing the soundclip (given as an audio file path) 
        TODO: Requires understanding of hardware mechanics. -> add
              to GPIO board somehow rather than just playing it to Linux.
              (We need our GPIO-connected speaker to somehow be 
               recognized as a sound output device by the Pi?)
        '''
        if not mixer.music.get_busy():
            # Won't restart from beginning if it's already playing
            # (prevent startSound to be called multiple times in the same second)
            mixer.music.load(soundclip)
            mixer.music.play()
            #GPIO.output(self.leftChannel, GPIO.HIGH)

class Display(object):
    ''' 
    A wrapper for the display hardware to allow basic controls
    in Python. Includes basic time-displaying functionalities. 

    Future dreams (TODO):
    - sleep progress bar (sleep tracking! If you are awake to see this, that's a probleM!)
    - display real time instead of alarm time? or next to? (one can be smaller than the other)
    - be able to make 2D array keyboard controllable by set alarm button, in order to config the
      wifi
    '''
    # Stored Time Globals
    currentTime   = [0,0,0,0] # The current time to display
    alarmTime     = [1,0,0,0] # The stored display/alarm time, in a readable 
                            # format (should contain 4 int digits, in 24-hour time)
    tempAlarmTime = [0,0,0,0] # Temporary storage for the alarmTime while editing 
                              # (allows resetting)
    
    # Display globals
    digitToImg = { # Number to PIL Image file
        1:None,2:None,3:None,4:None,5:None,6:None,7:None,8:None,9:None,0:None,'clear':None,'colon':None
        }
    screenWidth  = 160
    screenHeight = 128

    # Editing Interface/Button Globals
    editModeInd = -1 # Blinky time-editting mode, index matches the digit that should 
                     # be blinking (unless -1, in which case editing mode is off)
    buttonDelay = 0.3 # The amount of time before another button press is allowed, to prevent multiple presses
    buttonTime = 0.0  # A time tracker for the button

    def __init__(self,channels):
        ''' 
        Initiates the display hardware and begins rendering 
        on the display (in a separate thread).
        '''
        # Pins
        #BCMconversions = {22:25,36:16,16:23}
        self.screen = ST7735_TFT(
            port = 0,
            cs   = 0,
            dc   = channels['DC'],
            backlight = channels['BACKLIGHT'],
            rst  = channels['RST'],
            spi_speed_hz = 24000000,#,#
            width = self.screenHeight,#128, #160, 
            height= self.screenWidth,#160, #128  
            offset_left=0, 
            offset_top=0,
            rotation=0
        )
        #self.nightLight = channels['NIGHTLIGHT'] # TODO: Comment out, LED is for testing only
        #GPIO.setup(self.nightLight, GPIO.OUT, initial=GPIO.LOW)
        #GPIO.output(self.nightLight, GPIO.HIGH) # FOR TESTING ONLY
        #time.sleep(2)
        #GPIO.output(self.nightLight, GPIO.LOW) 

        # Set all the digits appropriately given the files
        for num in self.digitToImg.keys():
            filepath = './digit_sheet/'+str(num)+'.png'
            self.digitToImg[num] = Image.open(filepath)

        # Start rendering the display
        self.startRendering()

    # EDITING MODE FUNCS (serve as callbacks to setAlarm button inputs)
    def buttonWrapper(buttonFunc):
        ''' 
        Decorater func to deal with button timings and 
        prevent unintended double-clicking 
        '''
        def wrapper(self,callback_channel):
            if GPIO.input(callback_channel):
                # Is rising
                if self.buttonTime >= self.buttonDelay:
                    # Button is only called if past the falling-edge delay
                    # (prevents double-clicking)
                    buttonFunc(self,callback_channel)
                self.buttonTime = 0.
            else:
                # Is falling, starts the button timer
                self.buttonTime = time.time()
        return wrapper

    @buttonWrapper
    def startEditingMode(self,callback_channel):
        '''
        Start the editing mode by blinking the first digit on and off. 
        Can also stop the editing mode instead if it stops.
        '''
        if self.editModeInd == -1:
            # Editing mode was off, intialize it
            self.editModeInd = 0
            self.tempAlarmTime = self.alarmTime
        else:
            # Editing mode was on, turn it off
            self.editModeInd = -1

    @buttonWrapper
    def incrementDigit(self,callback_channel):
        ''' Increments a digit during the editing mode '''
        if self.editModeInd != -1:
            modNum = 3 if self.editModeInd==0 else \
                    10 if (self.editModeInd == 1 or self.editModeInd == 3) else 6
            self.alarmTime[self.editModeInd] = (self.alarmTime[self.editModeInd] + 1) % modNum

    @buttonWrapper
    def decrementDigit(self,callback_channel):
        ''' Decrements a digit during the editing mode '''
        if self.editModeInd != -1:
            modNum = 3 if self.editModeInd==0 else \
                    10 if (self.editModeInd == 1 or self.editModeInd == 3) else 6
            self.alarmTime[self.editModeInd] = (self.alarmTime[self.editModeInd] - 1) % modNum

    @buttonWrapper
    def selectLeftDigit(self,callback_channel):
        ''' Edits the digit to the left now instead '''
        if self.editModeInd != -1:
            self.editModeInd = (self.editModeInd - 1) % len(self.alarmTime)


    @buttonWrapper
    def selectRightDigit(self,callback_channel):
        ''' Edits digit to the right now instead '''
        if self.editModeInd != -1:
            self.editModeInd = (self.editModeInd + 1) % len(self.alarmTime)

    @buttonWrapper
    def resetEdits(self,callback_channel):
        ''' Resets the alarmTime to the value it was before any edits happened '''
        self.alarmTime = self.tempAlarmTime

    def isEditing(self,):
        ''' Returns true if editing mode is on '''
        return self.editModeInd != -1

    # GENERAL RENDERING AND FUNCS
    def startRendering(self,):
        ''' 
        Starts rendering the display in a separate THREAD, using a 
        given refresh rate. Also creates a blinky effect on the 
        position self.editModeInd when editing mode is enabled. 
        '''
        th = threading.Thread(target=self.renderingThread)
        th.start()

    def renderingThread(self,):
        ''' Start screen rendering application. '''
        blinkDelay = .5 # .5 second blink delay
        isOdd = False
        while True:
            
            if self.editModeInd != -1:
                # Editing mode is enabled, render alarm time and
                # make the current digit being edited digit blink by
                # adding it on every second loop
                alarmTime = copy.deepcopy(self.alarmTime)
                if isOdd:
                    alarmTime[self.editModeInd] = 'clear'
                    isOdd = False
                else:
                    isOdd = True
                self.renderAlarmTime(alarmTime)
                time.sleep(blinkDelay)
            else:
                # Normal mode, display current time instead
                self.renderAlarmTime(self.currentTime)
                time.sleep(1)

    def getAlarmTime(self,):
        return self.alarmTime

    def updateCurrentTime(self,newCurrTime):
        '''
        Updates the current time with newCurrTime
        '''
        self.currentTime = newCurrTime

    def updateAlarmTime(self,newAlarmTime):
        ''' 
        Updates the real display time (the alarm time that 
        is stored) according to the given input 
        '''
        self.alarmTime = newAlarmTime

    def createDigitBitmap(self,digit):
        ''' 
        Creates a digit bitmap for the given digit or character,
        using the self.digitToImg dictionary
        (Note: the only additional character you'd really need for
        this is the colon, ":") 

        Inputs:
            digit (int): the digit to render. if None, then the
                         created bitmap should be all dark/blank

        Outputs:
            dbitmap (list) : a PIL image object
        '''
        return self.digitToImg[digit]

    def renderAlarmTime(self,alarmTime):
        '''
        Inputs:
            alarmTime (list) : list of four int digits, to render
                               on-screen in the appropriate order
        
        NOTE: if editMode==True, this should become blinky somehow
        '''
        # Convert the alarm time into a set of appropriate PIL image files
        adjAlarmTime = alarmTime[:2] + ["colon"] + alarmTime[2:] # Adjusted alarm time to include the colon
        imgList = [self.createDigitBitmap(d) for d in adjAlarmTime]

        # Combines the imgList items in a 
        # nice and correctly sized format with margin
        size = (self.screenWidth,self.screenHeight)
        timeImg = Image.new('RGB',size,color='#FFFFFF')
        vDist = int((size[1] - imgList[0].height) / 2)
        hStart = int((size[0] - sum([img.width for img in imgList])) / 2)
        timeImg.paste(imgList[0], (hStart, vDist)) 
        hDist = imgList[0].width + hStart
        for ind in range(1,len(imgList)):
            # Concatenates the images horizontally
            timeImg.paste(imgList[ind], (hDist, vDist))
            hDist += imgList[ind].width 

        # Apply to the display screen.
        timeImg = timeImg.transpose(Image.ROTATE_90) # rotate due to weird display weirdness
        #timeImg.save('currTime.jpg') # for TESTING only
        self.screen.display(timeImg)#draw(timeImg)


# OTHER ABSTRACTION CLASSES
class Time(object):
    ''' 
    A stored time object (as a list of 4 ints) in 24-hour time.
    Contains helpful operations, like comparators.

    TODO: pull in all time object data into here instead for 
    consistency and easy maintainability.
    '''
    pass


# BASE PYTHON CONTROLS
# (manages and coordinates all the above elements)
class AlarmClock(object):
    ''' 
    Class that coordinates and manages all 
    the hardware and life-manager features. 
    '''
    # Globals
    lastCheckedTime    = [] # Last checked time object (list of four ints)

    def __init__(self,speaker,display,inspbutton,setalarmbutton,onoffbutton):
        ''' 
        Initiates the AlarmClock class with the necessary hardware components.

        speaker       : The python wrapper object for the speaker (hardware)
        display       : The python wrapper object for the display (hardware)
        inspbutton    : The python wrapper for the "inspirational messages" button
        setalarmbutton: "" for the "set alarm" button
        onoffbutton   : "" for the button that can turn the alarm on and off
        '''
        self.speaker = speaker
        self.display = display
        self.inspbutton = inspbutton
        self.setalarmbutton = setalarmbutton
        self.onoffbutton = onoffbutton

    def main(self,):
        ''' Starts all the major alarm clock processes. '''

        # Check time for alarm sounding
        self.startTimeChecking()

    def startTimeChecking(self,):
        '''
        Starts a time checking polling thread. Upon the system time being 
        determined to be true, starts the alarm sound by pulling a random 
        audio file from the morning_msgs.
        '''
        th = threading.Thread(target=self.timeCheckingWorker)
        th.start()

    def timeCheckingWorker(self,):
        '''
        Starts a time checking polling thread. Upon the system time being 
        determined to be true, starts the alarm sound by pulling a random 
        audio file from the morning_msgs.
        '''
        while True:
            # Polling loop
            isTime = self.checkTime()
            isNotEditing = not self.display.isEditing()
            if isTime and isNotEditing:
                # Loop only does anything when we want the alarm to do something
            
                if self.onoffbutton.checkIsOn():
                    # Actually starts the audio 
                    self.speaker.playMorningAlarm()

            # Waits five seconds before polling again
            time.sleep(5)

    def getCurrSystemTime(self,):
        ''' 
        Gets the current system time, expressed as a list of four 
        integers (hours:minuts).
        '''
        currTime = datetime.datetime.now()
        currHr = currTime.hour
        currMin = currTime.minute
        time = [math.floor(currHr/10),currHr % 10,math.floor(currMin/10),currMin % 10]
        #print(time)
        return time

    def checkTime(self,):
        '''
        Checks if current system/online time matches the display timer. Returns
        True if it matches or if the the alarmTime is before the current time 
        and after the self.lastCheckedTime.
        
        OUTER CONTEXT: This function needs to be periodically called to function
        appropriately.

        TODO: Test transition from one 24-hour day to the next.
        '''
        def checkIfTimeIsLessThan(time,valueToCompareTo):
            ''' 
            Checks if the given time input is less than the 
            valueToCompareTo, given the [int,int,int,int] formatting
            of time. 
            '''
            for ind in range(len(time)):
                if time[ind] < valueToCompareTo[ind]:
                    return True
                if time[ind] > valueToCompareTo[ind]:
                    return False
            return False

        # Gets the current time and the alarm clock time
        currSystemTime = self.getCurrSystemTime()
        self.display.updateCurrentTime(currSystemTime) # update the display with the current time
        alarmTime = self.setalarmbutton.getAlarmTime()

        # Compares them and outputs message
        isTime = (alarmTime == currSystemTime) or \
                  (checkIfTimeIsLessThan(alarmTime,currSystemTime)
                  and checkIfTimeIsLessThan(self.lastCheckedTime,alarmTime))
        self.lastCheckedTime = currSystemTime
        return isTime       

if __name__ == '__main__':
    # Initiate and run
    speaker     = Speaker({'LEFT':32,'RIGHT':33})
    display     = Display({'NIGHTLIGHT':37,'DC':22,'RST':36,'BACKLIGHT':16})
    inspButton  = InspirationalButton(speaker,12)
    setAlarmButton  = SetAlarmButton(display,{'MID':10,'UP':29,'DOWN':31,'LEFT':7,'RIGHT':15})
    onOffButton     = OnOffSwitch(35)
    alarmClock = AlarmClock(speaker,display,inspButton,setAlarmButton,onOffButton)
    alarmClock.main()

    #display.updateAlarmTime([0,2,0,6]) # TESTING
    #display.screen.close() # TODO: Figure out the code and put this in a better spot
