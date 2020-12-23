from tools import force_reimport


def test_128_64_0(GPIO, spidev, numpy):
    force_reimport('ST7735')
    import ST7735
    display = ST7735.ST7735(port=0, cs=0, dc=24, width=128, height=64, rotation=0)
    assert display.width == 128
    assert display.height == 64


def test_128_64_90(GPIO, spidev, numpy):
    force_reimport('ST7735')
    import ST7735
    display = ST7735.ST7735(port=0, cs=0, dc=24, width=128, height=64, rotation=90)
    assert display.width == 64
    assert display.height == 128
