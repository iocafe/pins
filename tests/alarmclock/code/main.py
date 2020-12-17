'''
Project: Kail Birthday Present
Author(s): Pekka Lehtikoski and Sofie Lehtikoski
Description: An expandable life-manager device, starting 
             with an alarm clock and inspirational message 
             selector. :)
'''
import time


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
    pass

class SetAlarmButton(ButtonController):
    '''
    Button that sets the alarm time on the display and
    internal program. Interfaces with the display and directly
    feeds it instructions based on the given editing mode rules 
    below.
    
    Rules:
        - First press, changes into blinky edit mode.
        - First number blinks, short presses increment # upwards
        - Long press causes next digit to be selected (blinking)
        - When all digits were iterated over, exits out of the
          editting mode
    '''
    def __init__(self,display):
        self.display = display

    def startEditing(self,):
        ''' 
        Start editing the displayTime using above rules. 
        
        CONTEXT: Called on-event when the physical SetAlarmButton is first pressed.
        '''
        pass

    def getAlarmTime(self,):
        # Gets the stored alarm time displayed
        return self.display.getDisplayTime()
    

class OnOffButton(ButtonController):
    '''
    Button that is able to turn the alarm on and off.
    Does not need to interface with any other hardware,
    only the base program internals.
    '''
    isOn = True

    def switchOnOff(self,):
        ''' 
        Switches the value of isOn when the button is pressed.
        TODO: Requires knowledge of button mechanics or some
        event handling. I'll leave that to you, Pekka. -SKL
        '''
        pass

    def checkIsOn(self,):
        return self.isOn

class Speaker(object):
    ''' 
    A wrapper for the speaker hardware to allow basic controls 
    in Python.
    '''
    def startSound(self,soundclip):
        ''' 
        Starts playing the soundclip (given as an audio file path) 
        TODO: Requires understanding of hardware mechanics.
        '''
        pass

class Display(object):
    ''' 
    A wrapper for the display hardware to allow basic controls
    in Python. Includes basic time-displaying functionalities. 
    '''
    displayTime = [0,0,0,0] # The stored display/alarm time, in a readable 
                            # format (should contain 4 int digits, in 24-hour time)
    editMode = False # Blinky time-editting mode, TODO: Only one digit should be blinking at a time

    def __init__(self,):
        ''' 
        Initiates the display with a 00:00 number format 
        '''
        self.renderAlarmTime(self.displayTime)

    def getDisplayTime(self,):
        return self.displayTime

    def updateDisplayTime(self,newDisplayTime):
        ''' 
        Updates the real display time (the alarm time that 
        is stored) according to the given input 
        '''
        self.displayTime = newDisplayTime

    def createDigitBitmap(self,digit,size):
        ''' 
        Creates a digit bitmap for the given digit or character.
        (Note: the only additional character you'd really need for
        this is the colon, ":")

        Inputs:
            digit (int): the digit to render
            size (list): takes format [x,y] where x is the number
                         of pixels horizontally that the rendered
                         digit can take up and y is the number of
                         vertical pixels 

        Outputs:
            dbitmap (list) : a 3D RGB array that can be applied to 

        TODO: Requires understanding of how digits should be rendered.
        '''
        dbitmap = []
        return dbitmap

    def renderAlarmTime(self,alarmTime):
        '''
        Inputs:
            alarmTime (list) : list of four int digits, to render
                               on-screen in the appropriate order
        
        NOTE: if editMode==True, this should become blinky somehow
        '''
        # Convert the alarm time into a set of appropriate dbitmaps
        digitsize = [200,800] # TODO: hardcoded value, you'd want to be able
                              #       to dynamically read this based on the 
                              #       actual display size
        adjAlarmTime = alarmTime[:2] + [":"] + alarmTime[2:] # Adjusted alarm time to include the colon
        dbitmaplist = [self.createDigitBitmap(d,digitsize) for d in adjAlarmTime]

        # TODO: Somehow actually combine the dbitmaplist items in a 
        # nice and correctly sized format and apply to the display screen.


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
    inspirational_msgs = [
        # List of inspirational messages, as audio files to pull from
        'audiofiles/dummy.mp4' # TODO: Does not exist yet
    ] 
    morning_msgs       = [
        # List of wake-up messages played at the alarm times (also audio files)
    ] 
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

        # TODO: Make the below called in a thread so main process doesn't halt. 
        #       (This is necessary for allowing the inspiration button 
        #        functionality, etc. to work)
        self.startTimeChecking()

        # Add event handlers for other buttons, such as the inspirational 
        # message button below and the set alarm button (which should then call some 
        # main button event processing (ex. setalarmbutton.startEditing()) when the event occurs)
        # TODO: Add inspirational message functionality 
        # (just uses messageShuffle then speaker.startSound(audiofile) as shown below)
        #audiofile = self.messageShuffle(self.inspirational_msgs)
        #self.speaker.startSound(audiofile)

        

    def startTimeChecking(self,):
        '''
        Starts a time checking polling thread. Upon the system time being 
        determined to be true, starts the alarm sound by pulling a random 
        audio file from the morning_msgs.
        '''
        while True:
            # Polling loop
            if self.onoffbutton.checkIsOn():
                # Loop only does anything when we want the alarm to do something
                isTime = self.checkTime()

                if isTime:
                    # Actually starts the audio 
                    audiofile = self.messageShuffle(self.morning_msgs)
                    self.speaker.startSound(audiofile)

            # Waits thirty seconds before polling again
            time.sleep(30)

    def messageShuffle(self, messageList):
        ''' 
        Selects a message from the messageList 
        at random and outputs it 
        '''
        import random
        ind = random.randint(0,len(messageList)-1)
        return messageList[ind]

    def getCurrSystemTime(self,):
        ''' 
        Gets the current system time, expressed as a list of four 
        integers (hours:minuts). TODO - wifi connection?
        '''
        return [0,0,0,0]

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
        alarmTime = self.setalarmbutton.getAlarmTime()

        # Compares them and outputs message
        isTime = (alarmTime == currSystemTime) or \
                  (checkIfTimeIsLessThan(alarmTime,currSystemTime)
                  and checkIfTimeIsLessThan(self.lastCheckedTime,alarmTime))
        self.lastCheckedTime = currSystemTime
        return isTime

if __name__ == '__main__':
    # Initiate and run
    speaker     = Speaker()
    display     = Display()
    inspButton  = InspirationalButton()
    setAlarmButton  = SetAlarmButton(display)
    onOffButton     = OnOffButton()
    alarmClock = AlarmClock(speaker,display,inspButton,setAlarmButton,onOffButton)
    alarmClock.main()