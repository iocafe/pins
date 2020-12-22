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

#import st7735s as controller
import Python_ST7735.ST7735 as TFT
import Adafruit_GPIO.SPI as SPI

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
    feeds it instructions based on the given editing mode rules 
    below.
    
    Editing Mode Rules:
        - First press, changes into blinky edit mode.
        - First number blinks, short presses increment # upwards
        - Long press causes next digit to be selected (blinking)
        - When all digits were iterated over, exits out of the
          editting mode
    '''
    def __init__(self,display,channels):
        # Initialize display
        self.display = display

        # SET UP GPIO PINs for the given button
        #
        # Start editing mode button
        midChnl = channels['set'] # SET button
        GPIO.setup(midChnl, GPIO.IN, pull_up_down=GPIO.PUD_DOWN) 
        GPIO.add_event_detect(midChnl, GPIO.RISING) 
        GPIO.add_event_callback(midChnl, self.display.startEditingMode) 
        #if GPIO.input(10):
        #GPIO.wait_for_edge(10, GPIO.RISING)

    def startEditing(self,):
        ''' 
        Start editing the alarmTime using above rules. 
        
        CONTEXT: Called on-event when the physical SetAlarmButton is first pressed.
        '''
        pass

    def getAlarmTime(self,):
        # Gets the stored alarm time displayed
        return self.display.getAlarmTime()
    
class OnOffSwitch(ButtonController):
    '''
    TOGGLE SWITCH that is able to turn the alarm on and off.
    Does not need to interface with any other hardware,
    only the base program internals.
    '''
    isOn = True # TODO: Delete when GpIO is actually connected
    def __init__(self,channel):
        ''' TODO: Set channel # '''
        self.chnnl = channel
        GPIO.setup(self.chnnl, GPIO.IN, pull_up_down=GPIO.PUD_DOWN) 
        #GPIO.input(chnnl)

    def checkIsOn(self,):
        #print('GPIO input')
        #print(GPIO.input(self.chnnl))
        return self.isOn #TODO: GPIO.input(self.chnnl)

class Speaker(object):
    ''' 
    A wrapper for the speaker hardware to allow basic controls 
    in Python. Includes controls for grabbing the audioclip to play.

    Future dreams:
    - connect speaker via bluetooth insteadof GPIO?
    - connect speaker to webradio for a radio or music option
    '''
    inspirational_msgs = [
        # List of inspirational messages, as audio files to pull from
        './audiofiles/wake up warrior kail.mp3',#dummy.mp3' # TODO: Does not exist yet
    ] 
    morning_msgs       = [
        # List of wake-up messages played at the alarm times (also audio files)
        '/coderoot/pins/tests/alarmclock/code/audiofiles/wake up warrior kail.mp3',
    ] 

    def __init__(self,channels):
        ''' Sets up the Speaker pi channel '''
        # Raspberry Pi GPIO stuff
        #self.leftChannel = channels["LEFT"] # Pins will be configured in 
                                            # operating system for auto-detection, not here
        #GPIO.setup(self.leftChannel, GPIO.OUT, initial=GPIO.LOW)

        # Sound play-back setup
        mixer.init()

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

    Future dreams:
    - sleep progress bar (sleep tracking! If you are awake to see this, that's a probleM!)
    - display real time instead of alarm time? or next to? (one can be smaller than the other)
    - be able to make 2D array keyboard controllable by set alarm button, in order to config the
      wifi
    '''
    currentTime = [0,0,0,0] # TODO, get current time and render it here
    alarmTime = [0,0,0,0] # The stored display/alarm time, in a readable 
                            # format (should contain 4 int digits, in 24-hour time)
    tempAlarmTime = [0,0,0,0] # Temporary storage for the alarmTime while editing 
                                # (allows resetting)
    editModeInd = None # Blinky time-editting mode, index matches the digit that should 
                       # be blinking (unless None)
    digitToImg = { # Number to PIL Image file
        1:None,2:None,3:None,4:None,5:None,6:None,7:None,8:None,9:None,0:None,'clear':None,'colon':None
        }
    screenWidth  = 160
    screenHeight = 128

    def __init__(self,channels):
        ''' 
        Initiates the display with a 00:00 number format, TODO
        '''
        # Pins
        
        self.screen = TFT.ST7735(
            channels['DC'],
            rst=channels['RST'],
            spi=SPI.SpiDev(
                0,
                0,
                max_speed_hz=8000000),
            width = self.screenWidth,
            height= self.screenHeight
        ) #controller.ST7735S()
        self.screen.begin()
        self.screen.clear()
        self.nightLight = channels['LED'] # TODO: Comment out, LED is for testing only
        GPIO.setup(self.nightLight, GPIO.OUT, initial=GPIO.LOW)

        # Set all the digits appropriately given the files
        for num in self.digitToImg.keys():
            filepath = './digit_sheet/'+str(num)+'.png'
            self.digitToImg[num] = Image.open(filepath)

        # Start rendering the display
        self.startRendering()

    # EDITING MODE FUNCS (serve as callbacks to setAlarm button inputs)
    def startEditingMode(self,callback_channel):
        '''
        Start the editing mode by blinking the first digit on and off. 

        TODO: Currently just blinks the entire LED output once for testing. 
        This should be connected to the display.
        '''
        self.editModeInd = 0
        self.tempAlarmTime = self.alarmTime
        GPIO.output(self.nightLight, GPIO.HIGH) # FOR TESTING ONLY, TODO: comment out
        time.sleep(2)
        GPIO.output(self.nightLight, GPIO.LOW) 

    def incrementDigit(self,callback_channel):
        ''' Increments a digit during the editing mode '''
        self.alarmTime[self.editModeInd] = (self.alarmTime[self.editModeInd] + 1) % 10 

    def decrementDigit(self,callback_channel):
        ''' Decrements a digit during the editing mode '''
        self.alarmTime[self.editModeInd] = (self.alarmTime[self.editModeInd] - 1) % 10 

    def selectLeftDigit(self,callback_channel):
        ''' Edits the digit to the left now instead '''
        self.editModeInd -= 1

    def selectRightDigit(self,callback_channel):
        ''' Edits digit to the right now instead '''
        self.editModeInd += 1

    def stopEditingMode(self,callback_channel):
        ''' Stops the editing mode '''
        self.editModeInd = None

    def resetEdits(self,callback_channel):
        ''' Resets the alarmTime to the value it was before any edits happened '''
        self.alarmTime = self.tempAlarmTime 


    # GENERAL RENDERING AND FUNCS
    def startRendering(self,):
        ''' 
        TODO: Starts rendering the display in a separate THREAD, using a 
        given refresh rate. 
        
        Creates a blinky effect on the position self.editModeInd when 
        editing mode is enabled. 
        '''
        th = threading.Thread(target=self.renderingThread)
        th.start()

    def renderingThread(self,):
        ''' Start screen rendering application. '''
        blinkDelay = .5 # .5 second blink delay
        isOdd = False
        while True:
            
            if self.editModeInd is not None:
                # Editing mode is enabled, render alarm time and
                # make the current digit being edited digit blink by
                # adding it on every second loop
                alarmTime = self.alarmTime # TODO: See if this gets modified succesfully from inside the thread?
                if isOdd:
                    alarmTime[self.editModeInd] = 'clear'
                    isOdd = False
                else:
                    isOdd = True
                time.sleep(blinkDelay)
                self.renderAlarmTime(alarmTime)
            else:
                # Normal mode, display current time instead
                self.renderAlarmTime(self.currentTime)
                time.sleep(0.1)

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
        size = (self.screenWidth-1,self.screenHeight)
        timeImg = Image.new('RGB',size)
        vDist = (size[1] - imgList[0].height) / 2
        hStart = (size[0] - sum([img.width for img in imgList])) / 2
        timeImg.paste(imgList[0], (hStart, vDist)) 
        hDist = imgList[0].width + hStart
        for ind in range(1,len(imgList)):
            # Concatenates the images horizontally
            timeImg.paste(imgList[ind], (hDist, vDist))
            hDist += imgList[ind].width 

        # Apply to the display screen.
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
            if isTime:
                # Loop only does anything when we want the alarm to do something
            
                if self.onoffbutton.checkIsOn():
                    # Actually starts the audio 
                    self.speaker.playMorningAlarm()

            # Waits thirty seconds before polling again
            time.sleep(5)

    def getCurrSystemTime(self,):
        ''' 
        Gets the current system time, expressed as a list of four 
        integers (hours:minuts). TODO - wifi connection?
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
    speaker     = Speaker({'LEFT':18,'RIGHT':13})
    display     = Display({'LED':37,'DC':22,'RST':36,'LIGHT':16})
    inspButton  = InspirationalButton(speaker,10)
    setAlarmButton  = SetAlarmButton(display,{'set':15,'mid':32})
    onOffButton     = OnOffSwitch(35)
    alarmClock = AlarmClock(speaker,display,inspButton,setAlarmButton,onOffButton)
    alarmClock.main()

    #display.updateAlarmTime([0,2,0,6]) # TESTING
    #display.screen.close() # TODO: Put this in a better spot