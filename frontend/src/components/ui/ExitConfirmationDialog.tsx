import { AlertTriangle, X } from 'lucide-react'
import { Button } from './Button'
import { cn } from '@/utils/cn'

interface ExitConfirmationDialogProps {
  isOpen: boolean
  onClose: () => void
  onConfirm: () => void
}

const ExitConfirmationDialog = ({ isOpen, onClose, onConfirm }: ExitConfirmationDialogProps) => {
  if (!isOpen) return null

  return (
    <div className="fixed inset-0 z-50 flex items-center justify-center">
      {/* Backdrop */}
      <div 
        className="absolute inset-0 bg-black/50 backdrop-blur-sm transition-opacity"
        onClick={onClose}
      />
      
      {/* Dialog */}
      <div className={cn(
        "relative bg-white rounded-xl shadow-2xl border border-gray-200",
        "max-w-xl w-full mx-4 transform transition-all",
        "animate-in fade-in-0 zoom-in-95 duration-200"
      )}>
        {/* Header */}
        <div className="flex items-center justify-between p-6 pb-4">
          <div className="flex items-center space-x-3">
            <div className="flex items-center justify-center w-10 h-10 bg-red-100 rounded-full">
              <AlertTriangle className="w-5 h-5 text-red-600" />
            </div>
            <h2 className="text-lg font-semibold text-gray-900">Confirm Exit</h2>
          </div>
          <button
            onClick={onClose}
            className="p-2 text-gray-400 hover:text-gray-600 hover:bg-gray-100 rounded-lg transition-colors"
          >
            <X className="w-5 h-5" />
          </button>
        </div>
        
        {/* Content */}
        <div className="px-6 pb-6">
          <p className="text-gray-600 mb-6 leading-relaxed">
            Are you sure you want to exit the 'META INSTALLER'? This will close the browser window and terminate your session.
          </p>
          
          {/* Actions */}
          <div className="flex items-center justify-end space-x-3">
            <Button 
              variant="outline" 
              onClick={onClose}
              className="px-6"
            >
              Cancel
            </Button>
            <Button 
              variant="destructive" 
              onClick={onConfirm}
              className="px-6"
            >
              Exit Application
            </Button>
          </div>
        </div>
      </div>
    </div>
  )
}

export { ExitConfirmationDialog }
