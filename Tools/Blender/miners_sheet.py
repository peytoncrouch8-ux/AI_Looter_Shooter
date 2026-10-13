"""The Providence's dead as game models: Saved/ArtPreviews/RansomsRest/LookSheets/40_MinersGame.jpg, for the user's
phone (under 7000 px tall, so it sends at full size).

Composes the renders Art/Models/Creatures/DeadMiner.py, Foreman.py, Hiram.py and Thaddeus.py made with --preview
(Saved/ArtPreviews/RansomsRest/<Name>/) on the concept kit's sheet (Art/Backlog/Creatures/MinerConcepts.py). Run it after
the model scripts' previews:

    Tools/artrun.ps1 -Script Tools/Blender/miners_sheet.py
"""
import os

import looter_miners as mn
import looter_textures as lt

DIR = os.path.join(lt.REPO, 'Saved', 'ArtPreviews', 'RansomsRest')
OUT = os.path.join(DIR, 'LookSheets', '40_MinersGame.jpg')
LIMIT = 7000


def p(name, file):
    return os.path.join(DIR, name, file)


def compose(width):
    mc = mn.kit()
    s = mc.Sheet(width=width)
    s.title('40  The Providence\'s dead: the game models')
    s.lines('Built for the game from the picks of sheet 38 (dead miners A, Foreman A, Hiram and Thaddeus as shown). Rest pose = '
            'the game\'s idle;\nthe arms and props below are laid as the code lays them. Export-tested only: nothing imported.',
            size=34)
    s.heading('Dead miners: canvas and carbide')
    s.row([(p('DeadMiner', 'DeadMiner_pack.png'), 'A   B   C',
            'The three outfits (MI_DeadMiner_A, B, C), picks carried as the\ncode\'s calm carry lays them. B wears the cap.'),
           (p('DeadMiner', 'DeadMiner_gallery.png'), '',
            'In a gallery by ghost light, their lamps lit (the code\'s glow\nstands in here): the weak spot reads at range.')])
    s.image(p('DeadMiner', 'DeadMiner_turn.png'),
            'SK_DeadMiner 7.9k at rest (the Unpaid\'s bones and idle, the right fist closed), beside a 1.8 m post; the felt 0.8k,\n'
            'the cap 0.6k, the lamp 1.0k (shown 2.5x), the pick 0.6k, the pail 0.5k. As drawn: 10.3k with the felt.')
    s.heading('The Foreman: the company man')
    s.image(p('Foreman', 'Foreman_turn.png'),
            'SK_Foreman 11.9k at the actor\'s 1.5x, beside a 1.8 m post and figure; bowler 1.3k, lamp 1.0k, handle 0.4k, satchel\n'
            '1.5k, fuse stick 0.2k: 16.1k as drawn. Abel\'s skirt bones carry the overcoat\'s skirts.')
    s.row([(p('Foreman', 'Foreman_golden.png'), '', 'At the Providence\'s mouth, the handle carried\nas the code\'s calm lays it.'),
           (p('Foreman', 'Foreman_fuse.png'), '', '"Fire in the hole!": the fuse lit in his raised left\nfist, the handle low, his coal open.')])
    s.heading('Hiram Coyle, the lift\'s engineman')
    s.image(p('Hiram', 'Hiram_turn.png'),
            'SK_Hiram 11.7k (the head 3.1k), his rest sat on his 56 cm stool; the cap with its goggles 1.0k. Right: the tube pose.')
    s.row([(p('Hiram', 'Hiram_golden.png'), '', 'On his stool by the cold boiler.'),
           (p('Hiram', 'Hiram_face.png'), '', 'His face at rest and talking: grey mutton chops.')])
    s.heading('Thaddeus Bright, the undertaker')
    s.image(p('Thaddeus', 'Thaddeus_turn.png'),
            'SK_Thaddeus 10.8k (the head 3.1k), his rest the measure; spectacles and pencil 0.5k. Right: the look and the knock.')
    s.row([(p('Thaddeus', 'Thaddeus_golden.png'), '', 'At his trestles in the cold room, measuring a coffin\nthat isn\'t there.'),
           (p('Thaddeus', 'Thaddeus_face.png'), '', 'His face looking up, and talking over his work.')])
    return s


sheet = compose(2000)
if sheet.y + sheet.pad > LIMIT:
    # Too tall to send at full size: lay it out narrower, so every picture is shorter.
    sheet = compose(int(2000 * (LIMIT - 200) / (sheet.y + sheet.pad)))
sheet.render(OUT)
