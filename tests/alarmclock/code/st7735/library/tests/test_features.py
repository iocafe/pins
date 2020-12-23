import mock
from tools import force_reimport


def test_display(GPIO, spidev, numpy):
    force_reimport('ST7735')
    import ST7735
    display = ST7735.ST7735(port=0, cs=0, dc=24)
    numpy.dstack().flatten().tolist.return_value = [0xff, 0x00, 0xff, 0x00]
    display.display(mock.MagicMock())

    spidev.SpiDev().xfer3.assert_called_with([0xff, 0x00, 0xff, 0x00])


def test_color565(GPIO, spidev, numpy):
    force_reimport('ST7735')
    import ST7735
    assert ST7735.color565(255, 255, 255) == 0xFFFF


def test_image_to_data(GPIO, spidev, numpy):
    force_reimport('ST7735')
    numpy.dstack().flatten().tolist.return_value = []
    import ST7735
    assert ST7735.image_to_data(mock.MagicMock()) == []
