///////////////////////////////////////////////////////////////////////////
// Name:        src/generic/gridsel.cpp
// Purpose:     wxGridSelection
// Author:      Stefan Neis
// Modified by:
// Created:     20/02/1999
// Copyright:   (c) Stefan Neis (Stefan.Neis@t-online.de)
// Licence:     wxWindows licence
/////////////////////////////////////////////////////////////////////////////

// For compilers that support precompilation, includes "wx/wx.h".
#include "wx/wxprec.h"

#ifdef __BORLANDC__
    #pragma hdrstop
#endif

// ============================================================================
// declarations
// ============================================================================

// ----------------------------------------------------------------------------
// headers
// ----------------------------------------------------------------------------

#if wxUSE_GRID

#include "wx/generic/gridsel.h"
#include "wx/hashset.h"


WX_DECLARE_HASH_SET(int, wxIntegerHash, wxIntegerEqual, wxIntSet);

// ----------------------------------------------------------------------------
// private helpers
// ----------------------------------------------------------------------------

namespace
{

    // ensure that first is less or equal to second, swapping the values if
    // necessary
    void EnsureFirstLessThanSecond(int& first, int& second)
    {
        if ( first > second )
            wxSwap(first, second);
    }

    void FixBlock(wxGridBlockCoords& block)
    {
        if ( block.GetTopRow() > block.GetBottomRow() )
        {
            int tmp = block.GetTopRow();
            block.SetBottomRow(block.GetBottomRow());
            block.SetTopRow(tmp);
        }
        if ( block.GetLeftCol() > block.GetRightCol() )
        {
            int tmp = block.GetLeftCol();
            block.SetLeftCol(block.GetRightCol());
            block.SetRightCol(tmp);
        }
    }

    wxVectorGridBlockCoords BlocksDifference(const wxGridBlockCoords& block1,
                                             const wxGridBlockCoords& block2)
    {
        wxVectorGridBlockCoords result;

        // Check whether the blocks intersects.
        if ( block1.GetTopRow() > block2.GetBottomRow() ||
             block1.GetBottomRow() < block2.GetTopRow() ||
             block1.GetLeftCol() < block2.GetRightCol() ||
             block1.GetRightCol() < block2.GetLeftCol() )
        {
            result.push_back(block1);
            result.push_back(block2);
            return result;
        }

        // Possible blocks:
        // |-----------------------|
        // |       |       |       | minUpper->GetTopRow()
        // |   1   |   2   |   3   |
        // |       |       |       | maxUpper->GetTopRow() - 1
        // |-----------------------|
        // |       |       |       | maxUpper->GetTopRow()
        // |   4   |   x   |   5   |
        // |       |       |       | minLower->GetBottomRow()
        // |-----------------------|
        // |       |       |       | minLower->GetBottomRow() + 1
        // |   6   |   7   |   8   |
        // |       |       |       | maxLower->GetBottomRow()
        // |-----------------------|

        // Blocks 1, 2, 3 - merged to one block.
        int maxUpperRow;
        if ( block1.GetTopRow() != block2.GetTopRow() )
        {
            const bool block1Min = block1.GetTopRow() < block2.GetTopRow();
            const wxGridBlockCoords& minUpper = block1Min ? block1 : block2;
            const wxGridBlockCoords& maxUpper = block1Min ? block2 : block1;
            maxUpperRow = maxUpper.GetTopRow();

            result.push_back(wxGridBlockCoords(minUpper.GetTopRow(),
                                               minUpper.GetLeftCol(),
                                               maxUpper.GetTopRow() - 1,
                                               minUpper.GetRightCol()));
        }
        else
        {
            maxUpperRow = block1.GetTopRow();
        }

        // Blocks 6, 7, 8 - merged to one block.
        int minLowerRow;
        if ( block1.GetBottomRow() != block2.GetBottomRow() )
        {
            const bool block1Min = block1.GetBottomRow() < block2.GetBottomRow();
            const wxGridBlockCoords& minLower = block1Min ? block1 : block2;
            const wxGridBlockCoords& maxLower = block1Min ? block2 : block1;
            minLowerRow = minLower.GetBottomRow();

            result.push_back(wxGridBlockCoords(minLower.GetBottomRow() + 1,
                                               maxLower.GetLeftCol(),
                                               maxLower.GetBottomRow(),
                                               maxLower.GetRightCol()));
        }
        else
        {
            minLowerRow = block1.GetBottomRow();
        }

        // Block 4.
        if ( block1.GetLeftCol() != block2.GetLeftCol() )
        {
            result.push_back(wxGridBlockCoords(maxUpperRow,
                                               wxMin(block1.GetLeftCol(),
                                                     block2.GetLeftCol()),
                                               minLowerRow,
                                               wxMax(block1.GetLeftCol(),
                                                     block2.GetLeftCol()) - 1));
        }

        // Block 5.
        if ( block1.GetRightCol() != block2.GetRightCol() )
        {
            result.push_back(wxGridBlockCoords(maxUpperRow,
                                               wxMin(block1.GetRightCol(),
                                                     block2.GetRightCol()) + 1,
                                               minLowerRow,
                                               wxMax(block1.GetRightCol(),
                                                     block2.GetRightCol())));
        }

        return result;
    }

} // anonymous namespace

wxGridSelection::wxGridSelection( wxGrid * grid,
                                  wxGrid::wxGridSelectionModes sel )
{
    m_grid = grid;
    m_selectionMode = sel;
}

bool wxGridSelection::IsSelection()
{
    return !m_selection.empty();
}

bool wxGridSelection::IsInSelection( int row, int col )
{
    // Check whether the given cell is contained in one of the selected blocks.
    size_t count = m_selection.size();
    for ( size_t n = 0; n < count; n++ )
    {
        wxGridBlockCoords& block = m_selection[n];
        if ( BlockContainsCell(block.GetTopRow(), block.GetLeftCol(),
                               block.GetBottomRow(), block.GetRightCol(),
                               row, col ) )
            return true;
    }

    return false;
}

// Change the selection mode
void wxGridSelection::SetSelectionMode( wxGrid::wxGridSelectionModes selmode )
{
    // if selection mode is unchanged return immediately
    if (selmode == m_selectionMode)
        return;

    // TODO: wxGridSelectRowsOrColumns?

    if ( m_selectionMode != wxGrid::wxGridSelectCells )
    {
        // if changing form row to column selection
        // or vice versa, clear the selection.
        if ( selmode != wxGrid::wxGridSelectCells )
            ClearSelection();

        m_selectionMode = selmode;
    }
    else
    {
        // Note that m_blockSelectionTopLeft's size may be changing!
        for ( size_t n = m_selection.size(); n > 0; )
        {
            n--;
            wxGridBlockCoords& block = m_selection[n];
            const int topRow = block.GetTopRow();
            const int leftCol = block.GetLeftCol();
            const int bottomRow = block.GetBottomRow();
            const int rightCol = block.GetRightCol();

            if (selmode == wxGrid::wxGridSelectRows)
            {
                if (leftCol != 0 || rightCol != m_grid->GetNumberCols() - 1 )
                {
                    m_selection.erase(m_selection.begin() + n);
                    SelectBlockNoEvent( topRow, 0,
                                 bottomRow, m_grid->GetNumberCols() - 1);
                }
            }
            else // selmode == wxGridSelectColumns)
            {
                if (topRow != 0 || bottomRow != m_grid->GetNumberRows() - 1 )
                {
                    m_selection.erase(m_selection.begin() + n);
                    SelectBlockNoEvent(0, leftCol,
                                 m_grid->GetNumberRows() - 1, rightCol);
                }
            }
        }

        m_selectionMode = selmode;
    }
}

void wxGridSelection::SelectRow(int row, const wxKeyboardState& kbd)
{
    if ( m_selectionMode == wxGrid::wxGridSelectColumns )
        return;

    Select(row, 0, row, m_grid->GetNumberCols() - 1, kbd, true);
}

void wxGridSelection::SelectCol(int col, const wxKeyboardState& kbd)
{
    if ( m_selectionMode == wxGrid::wxGridSelectRows )
        return;

    Select(0, col, m_grid->GetNumberRows() - 1, col, kbd, true);
}

void wxGridSelection::SelectBlock( int topRow, int leftCol,
                                   int bottomRow, int rightCol,
                                   const wxKeyboardState& kbd,
                                   bool sendEvent )
{
    // Fix the coordinates of the block if needed.
    switch ( m_selectionMode )
    {
        default:
            wxFAIL_MSG( "unknown selection mode" );
            wxFALLTHROUGH;

        case wxGrid::wxGridSelectCells:
            // nothing to do -- in this mode arbitrary blocks can be selected
            break;

        case wxGrid::wxGridSelectRows:
            leftCol = 0;
            rightCol = m_grid->GetNumberCols() - 1;
            break;

        case wxGrid::wxGridSelectColumns:
            topRow = 0;
            bottomRow = m_grid->GetNumberRows() - 1;
            break;

        case wxGrid::wxGridSelectRowsOrColumns:
            // block selection doesn't make sense for this mode, we could only
            // select the entire grid but this wouldn't be useful
            return;
    }

    if ( topRow > bottomRow )
    {
        int temp = topRow;
        topRow = bottomRow;
        bottomRow = temp;
    }

    if ( leftCol > rightCol )
    {
        int temp = leftCol;
        leftCol = rightCol;
        rightCol = temp;
    }

    Select(topRow, leftCol, bottomRow, rightCol, kbd, sendEvent);
}

void wxGridSelection::SelectCell( int row, int col,
                                  const wxKeyboardState& kbd,
                                  bool sendEvent )
{
    int topRow = row;
    int leftCol = col;
    int bottomRow = row;
    int rightCol = col;

    // Fix the coordinates if needed.
    switch (m_selectionMode)
    {
    default:
        wxFAIL_MSG("unknown selection mode");
        wxFALLTHROUGH;

    case wxGrid::wxGridSelectCells:
        // nothing to do -- in this mode arbitrary one-cell blocks can be selected
        break;

    case wxGrid::wxGridSelectRows:
        leftCol = 0;
        rightCol = m_grid->GetNumberCols() - 1;
        break;

    case wxGrid::wxGridSelectColumns:
        topRow = 0;
        bottomRow = m_grid->GetNumberRows() - 1;
        break;

    case wxGrid::wxGridSelectRowsOrColumns:
        // cell selection doesn't make sense for this mode
        return;
    }

    Select(topRow, leftCol, bottomRow, rightCol, kbd, sendEvent);
}

void
wxGridSelection::ToggleCellSelection(int row, int col,
                                     const wxKeyboardState& kbd)
{
    // if the cell is not selected, select it
    if ( !IsInSelection ( row, col ) )
    {
        SelectCell(row, col, kbd);

        return;
    }

    // otherwise deselect it. This can be simple or more or
    // less difficult, depending on how the cell is selected.
    size_t count, n;

    // The most difficult case: The cell is member of one or even several
    // blocks. Split each such block in up to 4 new parts, that don't
    // contain the cell to be selected, like this (for rows):
    // |---------------------------|
    // |                           |
    // |           part 1          |
    // |                           |
    // |---------------------------|
    // | part 3 |    x    | part 4 |
    // |---------------------------|
    // |                           |
    // |           part 2          |
    // |                           |
    // |---------------------------|
    // And for columns:
    // |---------------------------|
    // |        |         |        |
    // |        | part 3  |        |
    // |        |         |        |
    // |        |---------|        |
    // | part 1 |    x    | part 2 |
    // |        |---------|        |
    // |        |         |        |
    // |        | part 4  |        |
    // |        |         |        |
    // |---------------------------|
    //   (The x marks the newly deselected cell).
    // Note: in row/column selection mode, we only need part1 and part2

    // Blocks to refresh.
    wxVectorGridBlockCoords refreshBlocks;
    // Add the deselected cell.
    refreshBlocks.push_back(wxGridBlockCoords(row, col, row, col));

    count = m_selection.size();
    for ( n = 0; n < count; n++ )
    {
        const wxGridBlockCoords& block = m_selection[n];
        int topRow = block.GetTopRow();
        int leftCol = block.GetLeftCol();
        int bottomRow = block.GetBottomRow();
        int rightCol = block.GetRightCol();

        if ( BlockContainsCell( topRow, leftCol, bottomRow, rightCol, row, col ) )
        {
            // Add part3 and part4 only in the cells selection mode.
            bool addParts34 = m_selectionMode == wxGrid::wxGridSelectCells;

            bool splitHorizontally = true;
            switch ( m_selectionMode )
            {
            case wxGrid::wxGridSelectCells:
                if (block.GetLeftCol() == 0 &&
                    block.GetRightCol() == m_grid->GetNumberCols() - 1)
                    break;

                if (block.GetTopRow() == 0 &&
                    block.GetBottomRow() == m_grid->GetNumberRows() - 1)
                    splitHorizontally = false;

                break;

            case wxGrid::wxGridSelectColumns:
                splitHorizontally = false;
                break;

            case wxGrid::wxGridSelectRowsOrColumns:
                if (block.GetLeftCol() == 0 &&
                    block.GetRightCol() == m_grid->GetNumberCols() - 1)
                    break;

                splitHorizontally = false;
                break;
            }

            // remove the block
            m_selection.erase(m_selection.begin() + n);
            n--;
            count--;

            if ( splitHorizontally )
            {
                // Part1.
                if ( topRow < row )
                    SelectBlockNoEvent(topRow, leftCol, row - 1, rightCol);

                // Part2.
                if ( bottomRow > row )
                    SelectBlockNoEvent(row + 1, leftCol, bottomRow, rightCol);

                // Part3.
                if ( leftCol < col )
                {
                    if ( addParts34 )
                        SelectBlockNoEvent(row, leftCol, row, col - 1);
                    else
                        MergeOrAddBlock(refreshBlocks, row, leftCol, row, col - 1);
                }

                // Part4.
                if ( rightCol > col )
                {
                    if ( addParts34 )
                        SelectBlockNoEvent(row, col + 1, row, rightCol);
                    else
                        MergeOrAddBlock(refreshBlocks, row, col + 1, row, rightCol);
                }
            }
            else
            {
                // Part1.
                if ( leftCol < col )
                    SelectBlockNoEvent(topRow, leftCol, bottomRow, col - 1);

                // Part2.
                if ( rightCol > col )
                    SelectBlockNoEvent(topRow, col + 1, bottomRow, rightCol);

                // Part3.
                if ( topRow < row )
                {
                    if ( addParts34 )
                        SelectBlockNoEvent(topRow, col, row - 1, col);
                    else
                        MergeOrAddBlock(refreshBlocks, topRow, col, row - 1, col);
                }

                // Part4.
                if ( bottomRow > row )
                {
                    if ( addParts34 )
                        SelectBlockNoEvent(row + 1, col, bottomRow, col);
                    else
                        MergeOrAddBlock(refreshBlocks, row + 1, col, bottomRow, col);
                }
            }
        }
    }

    // Refresh the screen and send events.
    count = refreshBlocks.size();
    for ( n = 0; n < count; n++ )
    {
        const wxGridBlockCoords& block = refreshBlocks[n];

        if ( !m_grid->GetBatchCount() )
        {
            m_grid->RefreshBlock(block.GetTopRow(), block.GetLeftCol(),
                                 block.GetBottomRow(), block.GetRightCol());
        }

        wxGridRangeSelectEvent gridEvt(m_grid->GetId(),
                                       wxEVT_GRID_RANGE_SELECT,
                                       m_grid,
                                       block.GetTopLeft(),
                                       block.GetBottomRight(),
                                       false,
                                       kbd);
        m_grid->GetEventHandler()->ProcessEvent(gridEvt);
    }
}

void wxGridSelection::ClearSelection()
{
    size_t n;
    wxRect r;
    wxGridCellCoords coords1, coords2;

    // deselect all blocks and update the screen
    while ( ( n = m_selection.size() ) > 0)
    {
        n--;
        wxGridBlockCoords& block = m_selection[n];
        coords1 = block.GetTopLeft();
        coords2 = block.GetBottomRight();
        m_selection.erase(m_selection.begin() + n);
        if ( !m_grid->GetBatchCount() )
        {
            m_grid->RefreshBlock(coords1, coords2);

#ifdef __WXMAC__
            m_grid->UpdateGridWindows();
#endif
        }
    }

    // One deselection event, indicating deselection of _all_ cells.
    // (No finer grained events for each of the smaller regions
    //  deselected above!)
    wxGridRangeSelectEvent gridEvt( m_grid->GetId(),
                                    wxEVT_GRID_RANGE_SELECT,
                                    m_grid,
                                    wxGridCellCoords( 0, 0 ),
                                    wxGridCellCoords(
                                        m_grid->GetNumberRows() - 1,
                                        m_grid->GetNumberCols() - 1 ),
                                    false );

    m_grid->GetEventHandler()->ProcessEvent(gridEvt);
}


void wxGridSelection::UpdateRows( size_t pos, int numRows )
{
    size_t count = m_selection.size();
    size_t n;

    for ( n = 0; n < count; n++ )
    {
        wxGridBlockCoords& block = m_selection[n];
        wxCoord row1 = block.GetTopRow();
        wxCoord row2 = block.GetBottomRow();

        if ((size_t)row2 >= pos)
        {
            if (numRows > 0)
            {
                // If rows inserted, increase row counter where necessary
                block.SetBottomRow( row2 + numRows );
                if ((size_t)row1 >= pos)
                    block.SetTopRow( row1 + numRows );
            }
            else if (numRows < 0)
            {
                // If rows deleted ...
                if ((size_t)row2 >= pos - numRows)
                {
                    // ...either decrement row counter (if row still exists)...
                    block.SetBottomRow( row2 + numRows );
                    if ((size_t)row1 >= pos)
                        block.SetTopRow( wxMax(row1 + numRows, (int)pos) );

                }
                else
                {
                    if ((size_t)row1 >= pos)
                    {
                        // ...or remove the attribute
                        m_selection.erase(m_selection.begin() + n);
                        n--;
                        count--;
                    }
                    else
                        block.SetBottomRow( pos );
                }
            }
        }
    }
}


void wxGridSelection::UpdateCols( size_t pos, int numCols )
{
    size_t count = m_selection.size();
    size_t n;

    for ( n = 0; n < count; n++ )
    {
        wxGridBlockCoords& block = m_selection[n];
        wxCoord col1 = block.GetLeftCol();
        wxCoord col2 = block.GetRightCol();

        if ((size_t)col2 >= pos)
        {
            if (numCols > 0)
            {
                // If rows inserted, increase row counter where necessary
                block.SetRightCol(col2 + numCols);
                if ((size_t)col1 >= pos)
                    block.SetLeftCol(col1 + numCols);
            }
            else if (numCols < 0)
            {
                // If cols deleted ...
                if ((size_t)col2 >= pos - numCols)
                {
                    // ...either decrement col counter (if col still exists)...
                    block.SetRightCol(col2 + numCols);
                    if ( (size_t) col1 >= pos)
                        block.SetLeftCol( wxMax(col1 + numCols, (int)pos) );

                }
                else
                {
                    if ((size_t)col1 >= pos)
                    {
                        // ...or remove the attribute
                        m_selection.erase(m_selection.begin() + n);
                        n--;
                        count--;
                    }
                    else
                        block.SetRightCol(pos);
                }
            }
        }
    }
}

wxGridCellCoordsArray wxGridSelection::GetCellSelection() const
{
    if ( m_selectionMode != wxGrid::wxGridSelectCells )
        return wxGridCellCoordsArray();

    wxGridCellCoordsArray cells;
    const size_t count = m_selection.size();
    for ( size_t n = 0; n < count; n++ )
    {
        const wxGridBlockCoords& block = m_selection[n];
        if ( block.GetTopRow() == block.GetBottomRow() &&
             block.GetLeftCol() == block.GetRightCol() )
        {
            cells.Add(block.GetTopLeft());
        }
    }
    return cells;
}

wxGridCellCoordsArray wxGridSelection::GetBlockSelectionTopLeft() const
{
    // return blocks only in wxGridSelectCells selection mode
    if ( m_selectionMode != wxGrid::wxGridSelectCells )
        return wxGridCellCoordsArray();

    wxGridCellCoordsArray coords;
    const size_t count = m_selection.size();
    for ( size_t n = 0; n < count; n++ )
    {
        coords.Add(m_selection[n].GetTopLeft());
    }
    return coords;
}

wxGridCellCoordsArray wxGridSelection::GetBlockSelectionBottomRight() const
{
    if ( m_selectionMode != wxGrid::wxGridSelectCells )
        return wxGridCellCoordsArray();

    wxGridCellCoordsArray coords;
    const size_t count = m_selection.size();
    for ( size_t n = 0; n < count; n++ )
    {
        coords.Add(m_selection[n].GetBottomRight());
    }
    return coords;
}

wxArrayInt wxGridSelection::GetRowSelection() const
{
    if ( m_selectionMode == wxGrid::wxGridSelectColumns )
        return wxArrayInt();

    wxIntSet uniqueRows;
    const size_t count = m_selection.size();
    for ( size_t n = 0; n < count; ++n )
    {
        const wxGridBlockCoords& block = m_selection[n];
        if ( block.GetLeftCol() == 0 &&
             block.GetRightCol() == m_grid->GetNumberCols() - 1 )
        {
            for ( int r = block.GetTopRow(); r <= block.GetBottomRow(); ++r )
            {
                uniqueRows.insert(r);
            }
        }
    }

    wxArrayInt result;
    result.Alloc(uniqueRows.size());
    wxIntSet::iterator it;
    for( it = uniqueRows.begin(); it != uniqueRows.end(); ++it )
    {
        result.Add(*it);
    }
    return result;
}

wxArrayInt wxGridSelection::GetColSelection() const
{
    if ( m_selectionMode == wxGrid::wxGridSelectRows )
        return wxArrayInt();

    wxIntSet uniqueCols;
    const size_t count = m_selection.size();
    for ( size_t n = 0; n < count; ++n )
    {
        const wxGridBlockCoords& block = m_selection[n];
        if ( block.GetTopRow() == 0 &&
             block.GetBottomRow() == m_grid->GetNumberRows() - 1 )
        {
            for ( int c = block.GetLeftCol(); c <= block.GetRightCol(); ++c )
            {
                uniqueCols.insert(c);
            }
        }
    }

    wxArrayInt result;
    result.Alloc(uniqueCols.size());
    wxIntSet::iterator it;
    for( it = uniqueCols.begin(); it != uniqueCols.end(); ++it )
    {
        result.Add(*it);
    }
    return result;
}

int wxGridSelection::BlockContain( int topRow1, int leftCol1,
                                   int bottomRow1, int rightCol1,
                                   int topRow2, int leftCol2,
                                   int bottomRow2, int rightCol2 )
// returns 1, if Block1 contains Block2,
//        -1, if Block2 contains Block1,
//         0, otherwise
{
    if ( topRow1 <= topRow2 && bottomRow2 <= bottomRow1 &&
         leftCol1 <= leftCol2 && rightCol2 <= rightCol1 )
        return 1;
    else if ( topRow2 <= topRow1 && bottomRow1 <= bottomRow2 &&
              leftCol2 <= leftCol1 && rightCol1 <= rightCol2 )
        return -1;

    return 0;
}

void
wxGridSelection::Select(int topRow, int leftCol, int bottomRow, int rightCol,
                        const wxKeyboardState& kbd, bool sendEvent)
{
    if (m_grid->GetNumberRows() == 0 || m_grid->GetNumberCols() == 0)
        return;

    MergeOrAddBlock(m_selection, topRow, leftCol, bottomRow, rightCol);

    // Update View:
    if ( !m_grid->GetBatchCount() )
    {
        m_grid->RefreshBlock(topRow, leftCol, bottomRow, rightCol);
    }

    // Send Event, if not disabled.
    if ( sendEvent )
    {
        wxGridRangeSelectEvent gridEvt( m_grid->GetId(),
            wxEVT_GRID_RANGE_SELECT,
            m_grid,
            wxGridCellCoords( topRow, leftCol ),
            wxGridCellCoords( bottomRow, rightCol ),
            true,
            kbd);
        m_grid->GetEventHandler()->ProcessEvent( gridEvt );
    }
}

void wxGridSelection::MergeOrAddBlock(wxVectorGridBlockCoords& blocks,
                                      int topRow, int leftCol,
                                      int bottomRow, int rightCol)
{
    // If a block containing the selection is already selected, return,
    // if a block contained in the selection is found, remove it.

    size_t count = blocks.size();
    for ( size_t n = 0; n < count; n++ )
    {
        const wxGridBlockCoords& block = blocks[n];

        switch ( BlockContain(block.GetTopRow(), block.GetLeftCol(),
                              block.GetBottomRow(), block.GetRightCol(),
                              topRow, leftCol, bottomRow, rightCol) )
        {
            case 1:
                return;

            case -1:
                blocks.erase(blocks.begin() + n);
                n--;
                count--;
                break;

            default:
                break;
        }
    }

    blocks.push_back(wxGridBlockCoords(topRow, leftCol, bottomRow, rightCol));
}

#endif
