import mock
from tools import force_reimport


def test_setup(GPIO, spidev, numpy):
    force_reimport('ST7735')
    import ST7735
    display = ST7735.ST7735(port=0, cs=0, dc=24)
    del display

    GPIO.output.assert_has_calls([
        mock.call(24, True),
        mock.call(24, False)
    ], any_order=True)


def test_setup_no_invert(GPIO, spidev, numpy):
    force_reimport('ST7735')
    import ST7735
    display = ST7735.ST7735(port=0, cs=0, dc=24, invert=False)
    del display


def test_setup_with_backlight(GPIO, spidev, numpy):
    force_reimport('ST7735')
    import ST7735
    display = ST7735.ST7735(port=0, cs=0, dc=24, backlight=4)
    GPIO.setup.assert_called_with(4, GPIO.OUT)

    display.set_backlight(GPIO.HIGH)

    GPIO.output.assert_has_calls([
        mock.call(4, GPIO.LOW),
        mock.call(4, GPIO.HIGH),
        # Dozens of falls with True/False here
        # due to _init() being called and the display
        # setup setting the command/data pin
        mock.call(4, GPIO.HIGH)
    ], any_order=True)


def test_setup_with_reset(GPIO, spidev, numpy):
    force_reimport('ST7735')
    import ST7735
    display = ST7735.ST7735(port=0, cs=0, dc=24, rst=4)
    GPIO.setup.assert_called_with(4, GPIO.OUT)
    del display
