from __future__ import annotations

import itertools
import random
import ast

from enum import Enum

from typing import Any
from typing import Tuple

import pygame as pg

from pygame import Color
from pygame import Rect

from pygame.surface import Surface

from pygame.sprite import AbstractGroup
from pygame.sprite import Group
from pygame.sprite import RenderUpdates
from pygame.sprite import Sprite


class Direction(Enum):
  UP = (0, -1)
  DOWN = (0, 1)
  LEFT = (-1, 0)
  RIGHT = (1, 0)

  def opposite(self, other: Direction):
    return (self[0] + other[0], self[1] + other[1]) == (0, 0)

  def __getitem__(self, i: int):
    return self.value[i]


class SnakeHead(Sprite):

  def __init__(
      self,
      size: int,
      position: Tuple[int, int],
      facing: Direction,
      bounds: Rect,
  ) -> None:
    super().__init__()
    self.image = Surface((size, size))
    self.image.fill(Color("aquamarine4"))
    self.rect = self.image.get_rect()
    self.rect.center = position
    self.facing = facing
    self.size = size
    self.speed = size
    self.bounds = bounds

  def update(self, *args: Any, **kwargs: Any) -> None:
    # Move the snake in the direction it is facing.
    self.rect.move_ip((
        self.facing[0] * self.speed,
        self.facing[1] * self.speed,
    ))

    # Move to the opposite side of the screen if the snake goes out of bounds.
    if self.rect.right > self.bounds.right:
      self.rect.left = 0
    elif self.rect.left < 0:
      self.rect.right = self.bounds.right

    if self.rect.bottom > self.bounds.bottom:
      self.rect.top = 0
    elif self.rect.top < 0:
      self.rect.bottom = self.bounds.bottom

  def change_direction(self, direction: Direction):
    if not self.facing == direction and not direction.opposite(self.facing):
      self.facing = direction


class SnakeBody(Sprite):

  def __init__(
      self,
      size: int,
      position: Tuple[int, int],
      colour: str = "white",
  ) -> None:
    super().__init__()
    self.image = Surface((size, size))
    self.image.fill(Color(colour))
    self.rect = self.image.get_rect()
    self.rect.center = position


class Snake(RenderUpdates):

  def __init__(self, game: Game) -> None:
    self.segment_size = game.segment_size
    self.colours = itertools.cycle(["aquamarine1", "aquamarine3"])

    self.head = SnakeHead(
        size=self.segment_size,
        position=game.rect.center,
        facing=Direction.RIGHT,
        bounds=game.rect,
    )

    neck = [
        SnakeBody(
            size=self.segment_size,
            position=game.rect.center,
            colour=next(self.colours),
        ) for _ in range(2)
    ]

    super().__init__(*[self.head, *neck])

    self.body = Group()
    self.tail = neck[-1]

  def increase_speed(self, factor: float = 1.03):
    # Increase the snake's speed temporarily by a factor.
    self.head.speed *= factor

  def grow(self):
    # Grow the snake
    tail = SnakeBody(
        size=self.segment_size,
        position=self.tail.rect.center,
        colour=next(self.colours),
    )
    self.tail = tail
    self.add(self.tail)
    self.body.add(self.tail)

  def update(self, *args: Any, **kwargs: Any) -> None:
    self.head.update()

    # Update snake body sprites
    segments = self.sprites()
    for i in range(len(segments) - 1, 0, -1):
      # Current sprite takes the position of the previous sprite.
      segments[i].rect.center = segments[i - 1].rect.center

  def change_direction(self, direction: Direction):
    self.head.change_direction(direction)

  def grow(self):
    tail = SnakeBody(
        size=self.segment_size,
        position=self.tail.rect.center,
        colour=next(self.colours),
    )
    self.tail = tail
    self.add(self.tail)
    self.body.add(self.tail)


class SnakeFood(Sprite):

  def __init__(self, game: Game, size: int, *groups: AbstractGroup) -> None:
    super().__init__(*groups)
    self.image = Surface((size*1.5, size*1.5))
    self.image.fill(Color("red"))
    self.rect = self.image.get_rect()
    self.rect.inflate_ip(10, 10)  # Increase the size by 5 pixels in all directions


    self.rect.topleft = (
        random.randint(0, game.rect.width - 200),
        random.randint(0, game.rect.height - 200),
    )

    self.rect.clamp_ip(game.rect)

    # XXX: This approach to random food placement might end badly if the snake is very large.

    while pg.sprite.spritecollideany(self, game.snake):
      self.rect.topleft = (
          random.randint(0, game.rect.width),
          random.randint(0, game.rect.height),
      )

      self.rect.clamp_ip(game.rect)


class Game:

  def validate_student_id(self, student_id: str) -> bool:
    # Check if student ID is 8 char long and only digits
    return len(student_id) == 8 and student_id.isdigit()

  def get_student_id(self) -> str:
    while True:
      student_id = input("Enter your student ID: ")
      if self.validate_student_id(student_id):
        return student_id
      else:
        print("Error: Student ID must be 8 digits.")

  def __init__(self) -> None:
    self.rect = Rect(0, 0, 800, 600)
    self.background = Surface(self.rect.size)
    self.background.fill(Color("black"))

    self.score = 0
    self.framerate = 16

    self.segment_size = 10
    self.snake = Snake(self)
    self.food_group = RenderUpdates(
      SnakeFood(game=self, size=self.segment_size))

    self.high_scores = self.load_high_scores()  #loads scores from file
    
    pg.font.init()  # Initialize font module
    self.font = pg.font.SysFont('Arial', 12)
    
    

    pg.init()
    self.screen = self._init_display()

  

  def load_high_scores(self):
    try:
        with open('high_scores.txt', 'r') as file:
            high_scores = []
            for line in file:
                stripped_line = line.strip()
                if stripped_line:  # Check if the line is not empty
                    try:
                        score = ast.literal_eval(stripped_line)
                        high_scores.append(score)
                    except SyntaxError:
                        # Handle lines that can't be parsed
                        print(f"Skipping invalid line: {stripped_line}")
            return high_scores
    except FileNotFoundError:
        # File not found, return default scores
        return [(0, '00000000')] * 3


  def save_high_scores(self):
    with open('high_scores.txt', 'w') as file:
      for score in self.high_scores:
        file.write(str(score) + '\n')

  def update_high_scores(self, student_id: str):
    self.high_scores.append((self.score, student_id))
    self.high_scores.sort(reverse=True, key=lambda x: x[0])  # Sort scores in descending order
    self.high_scores = self.high_scores[:7]  # Keep only the top 7 scores


  def display_high_scores(self):
    print("High Scores:")
    for i, (score, student_id) in enumerate(self.high_scores, start=1):
      print(f"{i}. {score} - Student ID: {student_id}")

  def _init_display(self) -> Surface:
    bestdepth = pg.display.mode_ok(self.rect.size, 0, 32)
    screen = pg.display.set_mode(self.rect.size, 0, bestdepth)

    pg.display.set_caption("Snake")
    pg.mouse.set_visible(False)

    screen.blit(self.background, (0, 0))
    pg.display.flip()

    return screen

  def draw(self, screen: Surface):
    dirty = self.snake.draw(screen)
    pg.display.update(dirty)

    dirty = self.food_group.draw(screen)
    pg.display.update(dirty)
    
    
    

  def update(self, screen):
    self.food_group.clear(screen, self.background)
    self.food_group.update()
    self.snake.clear(screen, self.background)
    self.snake.update()

  def main(self) -> int:
    

    # Display high scores
    self.display_high_scores()

    # Get the student ID from the user
    student_id = self.get_student_id()
    
    screen = self._init_display()
    clock = pg.time.Clock()

    # Ensure the screen is updated before entering the game loop
    pg.display.flip()

    # Main game loop
    while self.snake.head.alive():
        for event in pg.event.get():
            if event.type == pg.QUIT or (event.type == pg.KEYDOWN and event.key in (pg.K_ESCAPE, pg.K_q)):
                return self.score

        # Handle key events for snake direction change
        self.handle_key_events(event)

        # Game logic and drawing updates
        self.game_logic(screen)
        self.draw(screen)
        clock.tick(self.framerate)
        
        if not self.snake.head.alive():
          self.game_over(self.score)  # Display "Game Over" and the final score

          # Wait for a few seconds before quitting
          pg.time.delay(5000)

          # Quit the game
          pg.quit()

          # Game ends, update and display high scores
          self.update_high_scores(student_id)
          self.display_high_scores()  # updated high scores
          self.save_high_scores()

    print(f"Final Score: {self.score}")

  def handle_key_events(self, event):
    if event.type == pg.KEYDOWN:
        if event.key == pg.K_RIGHT:
            self.snake.change_direction(Direction.RIGHT)
        elif event.key == pg.K_LEFT:
            self.snake.change_direction(Direction.LEFT)
        elif event.key == pg.K_UP:
            self.snake.change_direction(Direction.UP)
        elif event.key == pg.K_DOWN:
            self.snake.change_direction(Direction.DOWN)

  def game_logic(self, screen):
    # Detect collisions and update game state
    self.update(screen)

    # Snake eats food
    for food in pg.sprite.spritecollide(self.snake.head, self.food_group, dokill=False):
        food.kill()
        self.snake.grow()
        self.score += 1
        # Increase framerate and snake speed
        if self.score % 5 == 0:
            self.framerate += 1
        self.snake.increase_speed(factor=1.1)
        self.food_group.add(SnakeFood(self, self.segment_size))

    for body_segment in self.snake.body:
        if pg.sprite.collide_rect(self.snake.head, body_segment):
            self.snake.head.kill()
            return
  
  def game_over(self, score):
    font = pg.font.Font(None, 36)
    text = font.render("Game Over", True, (255, 255, 255))
    text_rect = text.get_rect(center=(self.screen.get_width() // 2, self.screen.get_height() // 2 - 30))

    score_text = font.render(f"Score: {score}", True, (255, 255, 255))
    score_rect = score_text.get_rect(center=(self.screen.get_width() // 2, self.screen.get_height() // 2 + 30))

    self.screen.blit(text, text_rect)
    self.screen.blit(score_text, score_rect)
    pg.display.flip()





if __name__ == "__main__":
  game = Game()
  score = game.main()
