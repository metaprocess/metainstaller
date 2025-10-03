import { useRef, InputHTMLAttributes } from 'react'
import { Folder, X } from 'lucide-react'
import { cn } from '@/utils/cn'
import { Button } from './Button'

interface FilePickerProps extends Omit<InputHTMLAttributes<HTMLInputElement>, 'type' | 'value'> {
  onClear?: () => void
  accept?: string
  showClearButton?: boolean
  value?: string
  placeholder?: string
}

const FilePicker = ({ 
  className, 
  onClear, 
  showClearButton = true, 
  accept, 
  value, 
  placeholder = 'Select a file...',
  onChange,
  ...props 
}: FilePickerProps) => {
  const fileInputRef = useRef<HTMLInputElement>(null)

  const handleClick = () => {
    fileInputRef.current?.click()
  }

  const handleInputChange = (e: React.ChangeEvent<HTMLInputElement>) => {
    if (onChange) {
      onChange(e)
    }
  }

  return (
    <div className="relative">
      <div className="relative">
        <input
          type="file"
          className="sr-only"
          ref={fileInputRef}
          accept={accept}
          onChange={handleInputChange}
          {...props}
        />
        <div
          className={cn(
            'flex items-center min-h-10 w-full rounded-md border border-gray-300 bg-white px-3 py-2 text-sm placeholder:text-gray-400',
            'focus-within:outline-none focus-within:ring-2 focus-within:ring-blue-500 focus-within:border-transparent',
            'hover:border-gray-400 transition-colors cursor-pointer',
            className
          )}
          onClick={handleClick}
        >
          <Folder className="h-4 w-4 text-gray-400 mr-2 flex-shrink-0" />
          <span className="flex-1 truncate">
            {value || placeholder}
          </span>
          {value && showClearButton && onClear && (
            <Button
              type="button"
              variant="ghost"
              size="sm"
              onClick={(e) => {
                e.stopPropagation()
                onClear()
              }}
              className="h-6 w-6 p-0 ml-2 hover:bg-gray-100"
            >
              <X className="h-3 w-3" />
            </Button>
          )}
        </div>
      </div>
      {!value && (
        <p className="text-xs text-gray-500 mt-1">
          Click to browse and select a file
        </p>
      )}
    </div>
  )
}

FilePicker.displayName = 'FilePicker'

export { FilePicker }