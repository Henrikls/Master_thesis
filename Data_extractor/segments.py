from datetime import timedelta

# Song segments with their start and end times (in minutes and seconds)
segments = [
    (timedelta(minutes=0, seconds=0), timedelta(minutes=1, seconds=44)),  # Precious Gems by Salon Dijon
    (timedelta(minutes=1, seconds=44), timedelta(minutes=4, seconds=30)),  # Runaway Quartet by Moments
    (timedelta(minutes=4, seconds=30), timedelta(minutes=6, seconds=44)),  # Game of Chess by Wicked Cinema
    (timedelta(minutes=6, seconds=44), timedelta(minutes=8, seconds=57)),  # Drunken Thrush by Cody Martin
    (timedelta(minutes=8, seconds=57), timedelta(minutes=11, seconds=28)), # Our Web Of Lies by Moments
    (timedelta(minutes=11, seconds=28), timedelta(minutes=13, seconds=50)),# Suite Du Matin by Moments
    (timedelta(minutes=13, seconds=50), timedelta(minutes=17, seconds=23)),# The Life We Had by Moments
    (timedelta(minutes=17, seconds=23), timedelta(minutes=20, seconds=32)),# Bell Tower by Wicked Cinema
    (timedelta(minutes=20, seconds=32), timedelta(minutes=22, seconds=7)), # Good Grace by Salon Dijon
    (timedelta(minutes=22, seconds=7), timedelta(minutes=26, seconds=19)), # A Black Tie Affair by Wicked Cinema
    (timedelta(minutes=26, seconds=19), timedelta(minutes=30, seconds=28)),# The Throne by Wicked Cinema
    (timedelta(minutes=30, seconds=28), timedelta(minutes=35, seconds=20)),# Waltz Of The Damned by JCar
    (timedelta(minutes=35, seconds=20), timedelta(minutes=39, seconds=0)), # Dorset Summer by Moments
    (timedelta(minutes=39, seconds=0), timedelta(minutes=40, seconds=43))  # Precious Gems by Salon Dijon
]

# Total video length (40:43)
total_video_length = timedelta(minutes=40, seconds=43)
