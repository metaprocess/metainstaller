import { useState } from 'react'
import { Power } from 'lucide-react'
import { Button } from './Button'
import { ExitConfirmationDialog } from './ExitConfirmationDialog'
import { cn } from '@/utils/cn'

interface ExitButtonProps {
  className?: string
}

const ExitButton = ({ className }: ExitButtonProps) => {
  const [isDialogOpen, setIsDialogOpen] = useState(false)

  const handleExitClick = () => {
    setIsDialogOpen(true)
  }

  const handleConfirmExit = () => {
    // Close the browser window/tab
    if (window.opener) {
      // If window was opened by another window, close it
      window.close()
    } else {
      // For standalone windows, try to close
      window.close()
      window.location.href = 'about:blank'
      
      // If close doesn't work (some browsers block this), show alternative message
      setTimeout(() => {
        alert('Please close this browser tab manually to exit the application.')
        // window.
      }, 100)
    }
  }

  const handleCloseDialog = () => {
    setIsDialogOpen(false)
  }

  return (
    <>
      <Button
        variant="destructive"
        size="lg"
        onClick={handleExitClick}
        className={cn(
          "gap-2 shadow-lg hover:shadow-xl transition-all duration-200",
          "bg-gradient-to-r from-red-600 to-red-700 hover:from-red-700 hover:to-red-800",
          "border-2 border-red-500 hover:border-red-600",
          "font-semibold text-base",
          "min-w-[120px]",
          className
        )}
      >
        <Power className="w-5 h-5" />
        EXIT
      </Button>

      <ExitConfirmationDialog
        isOpen={isDialogOpen}
        onClose={handleCloseDialog}
        onConfirm={handleConfirmExit}
      />
    </>
  )
}

export { ExitButton }
