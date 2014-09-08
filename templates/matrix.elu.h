/*
                    The HumbleHacker Keyboard Project
                 Copyright © 2008-2010, David Whetstone
               david DOT whetstone AT humblehacker DOT com

  This file is a part of The HumbleHacker Keyboard Project.

  The HumbleHacker Keyboard Project is free software: you can redistribute
  it and/or modify it under the terms of the GNU General Public License as
  published by the Free Software Foundation, either version 3 of the
  License, or (at your option) any later version.

  The HumbleHacker Keyboard Project is distributed in the hope that it will
  be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General
  Public License for more details.

  You should have received a copy of the GNU General Public License along
  with The HumbleHacker Keyboard Project.  If not, see
  <http://www.gnu.org/licenses/>.

*/

#ifndef __MATRIX_H__
#define __MATRIX_H__

#include <stdint.h>
#include <avr/io.h>

#define NUM_ROWS <%= kb.matrix.row_count %>
#define NUM_COLS <%= kb.matrix.col_count %>
#define NUM_CELLS <%= kb.matrix.col_count * kb.matrix.row_count %>

struct Cell
{
  uint8_t row;
  uint8_t col;
};

typedef struct Cell Cell;

static inline
uint8_t
cell_to_index(Cell cell)
{
  return cell.col * NUM_ROWS + cell.row;
}

<% if kb.block_ghost_keys then %>
#define BLOCK_GHOST_KEYS
<% end %>

static inline
void init_rows(void)
{
#if ARCH==ARCH_XMEGA
    /* with xmega, all pins can be permanently input with enabled pullup in
     * wired-and mode:
     *  writing 0 to OUT pulls it down to gnd
     *  writing 1 to OUT leaves it floating, therefore pulled
     *      high by enabled pullup
     */
<% for i,pin in ipairs(kb.row_pins) do %>
  PORT<%= string.sub(pin,2,2) %>.DIRCLR = <%= string.sub(pin,3,3) %>;
  PORT<%= string.sub(pin,2,2) %>.OUTSET = PIN<%= string.sub(pin,3,3) %>_bm;
  PORT<%= string.sub(pin,2,2) %>.PIN<%= string.sub(pin,3,3) %>CTRL= PORT_OPC_WIREDANDPULL_gc;
<% end %>
#endif
}

static inline
void
activate_row(uint8_t row)
{
#if ARCH==ARCH_XMEGA
#else
  // set all row pins as inputs
<% for i,pin in ipairs(kb.row_pins) do %>
  DDR<%= string.sub(pin,2,2) %> &= ~(1 << <%= pin %>);
<% end %>

  // set current row pin as output
  switch (row)
  {
<% for i,pin in ipairs(kb.row_pins) do %>
    case <%= i-1 %>: DDR<%= string.sub(pin,2,2) %> |= (1 << <%= pin %>); break;
<% end %>
  }
#endif

  // drive all row pins high (pullup)
#if ARCH==ARCH_XMEGA
<% for i,pin in ipairs(kb.row_pins) do %>
  PORT<%= string.sub(pin,2,2) %>.OUTSET = PIN<%= string.sub(pin,3,3) %>_bm;
<% end %>
#else
<% for i,pin in ipairs(kb.row_pins) do %>
  PORT<%= string.sub(pin,2,2) %> |= (1 << <%= pin %>);
<% end %>
#endif

  // drive current row pin low (no pullup)
  switch (row)
  {
#if ARCH==ARCH_XMEGA
<% for i,pin in ipairs(kb.row_pins) do %>
    case <%= i-1 %>: PORT<%= string.sub(pin,2,2) %>.OUTCLR = PIN<%= string.sub(pin,3,3) %>_bm; break;
<% end %>
#else
<% for i,pin in ipairs(kb.row_pins) do %>
    case <%= i-1 %>: PORT<%= string.sub(pin,2,2) %> &= ~(1 << <%= pin %>); break;
<% end %>
#endif
  }
}

static inline
uint32_t
read_row_data(void)
{
  uint32_t cols = 0;

#if ARCH==ARCH_XMEGA
<% for i,pin in ipairs(kb.col_pins) do %>
  if ((~PORT<%= string.sub(pin,2,2) %>.IN)&(PIN<%= string.sub(pin,3,3) %>_bm)) cols |= (1UL<< <%= i-1 %>);
<% end %>
#else
<% for i,pin in ipairs(kb.col_pins) do %>
  if ((~PIN<%= string.sub(pin,2,2) %>)&(1<<<%= pin %>)) cols |= (1UL<< <%= i-1 %>);
<% end %>
#endif

  return cols;
}


static inline
void
init_cols(void)
{
  /* Columns are inputs */
#if ARCH==ARCH_XMEGA
#else
<% for i,pin in ipairs(kb.col_pins) do %>
  DDR<%= string.sub(pin,2,2) %> &= ~(1 << <%= pin %>);
<% end %>
#endif

  /* Enable pull-up resistors on inputs */
#if ARCH==ARCH_XMEGA
#else
<% for i,pin in ipairs(kb.col_pins) do %>
  PORT<%= string.sub(pin,2,2) %> |= (1 << <%= pin %>);
<% end %>
#endif
}

#endif /* __MATRIX_H__ */

