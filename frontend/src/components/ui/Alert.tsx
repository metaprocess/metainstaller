import { HTMLAttributes, forwardRef } from 'react'
import { cn } from '@/utils/cn'

interface AlertProps extends HTMLAttributes<HTMLDivElement> {
  variant?: 'default' | 'destructive' | 'warning' | 'success'
}

const Alert = forwardRef<HTMLDivElement, AlertProps>(
  ({ className, variant = 'default', ...props }, ref) => {
    const variants = {
      default: 'bg-blue-50 text-blue-700 border-blue-200',
      destructive: 'bg-red-50 text-red-700 border-red-200',
      warning: 'bg-yellow-50 text-yellow-700 border-yellow-200',
      success: 'bg-green-50 text-green-700 border-green-200'
    }

    return (
      <div
        ref={ref}
        role="alert"
        className={cn(
          'relative w-full rounded-lg border p-4',
          variants[variant],
          className
        )}
        {...props}
      />
    )
  }
)
Alert.displayName = 'Alert'

const AlertDescription = forwardRef<HTMLParagraphElement, HTMLAttributes<HTMLParagraphElement>>(
  ({ className, ...props }, ref) => (
    <div
      ref={ref}
      className={cn('text-sm [&_p]:leading-relaxed', className)}
      {...props}
    />
  )
)
AlertDescription.displayName = 'AlertDescription'

export { Alert, AlertDescription }